// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGmshMetaFileReader.h"

#include "vtkCellType.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringFormatter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLUtilities.h"

#include "gmsh.h"

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

//-----------------------------------------------------------------------------
struct vtkGmshMetaFileReaderInternals
{
  struct TimeStepInfo
  {
    int GeometryIndex = -1;
    int FieldIndex = -1;
    double Value = 0.0;
  };

  struct FieldInfo
  {
    std::string Pattern;
    std::string ArrayName;
  };

  int NumberOfFiles = 1;
  std::string GeometryPattern;
  std::vector<FieldInfo> Fields;
  std::map<int, TimeStepInfo> TimeSteps;
  std::vector<double> TimeValues;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGmshMetaFileReader);

//-----------------------------------------------------------------------------
vtkGmshMetaFileReader::vtkGmshMetaFileReader()
  : Internals(new vtkGmshMetaFileReaderInternals)
{
  this->SetNumberOfInputPorts(0);
  this->PointDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkGmshMetaFileReader::Modified);
}

//-----------------------------------------------------------------------------
vtkGmshMetaFileReader::~vtkGmshMetaFileReader()
{
  this->SetFileName(nullptr);
}

//-----------------------------------------------------------------------------
vtkDataArraySelection* vtkGmshMetaFileReader::GetPointDataArraySelection()
{
  return this->PointDataArraySelection;
}

//-----------------------------------------------------------------------------
int vtkGmshMetaFileReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkGmshMetaFileReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkGmshMetaFileReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkGmshMetaFileReader::SetPointArrayStatus(const char* name, int status)
{
  this->PointDataArraySelection->SetArraySetting(name, status);
}

//-----------------------------------------------------------------------------
int vtkGmshMetaFileReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->ParseMetaFile())
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  const std::vector<double>& times = this->Internals->TimeValues;
  if (times.empty())
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  else
  {
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS(), times.data(), static_cast<int>(times.size()));
    const double range[2] = { times.front(), times.back() };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
  }
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
namespace
{
// The array name of a field file pattern: its file name up to the first '.' or
// '_', since time steps and iterations usually follow one of them.
std::string GetArrayName(const std::string& pattern)
{
  std::string name = vtksys::SystemTools::GetFilenameName(pattern);
  name = name.substr(0, name.find('.'));
  return name.substr(0, name.find('_'));
}

//-----------------------------------------------------------------------------
void ParseTimeSteps(vtkXMLDataElement* element,
  std::map<int, vtkGmshMetaFileReaderInternals::TimeStepInfo>& timeSteps, int& numberOfSteps,
  bool& hasValues)
{
  if (!element->GetScalarAttribute("number_of_steps", numberOfSteps))
  {
    numberOfSteps = 1;
  }
  int autoGenerate = 0;
  element->GetScalarAttribute("auto_generate_indices", autoGenerate);
  int indexIncrement = 1;
  element->GetScalarAttribute("increment_index_by", indexIncrement);
  int index = 0;
  element->GetScalarAttribute("start_index", index);
  double value = 0.0;
  hasValues = element->GetScalarAttribute("start_value", value) != 0;
  double valueIncrement = indexIncrement;
  hasValues |= element->GetScalarAttribute("increment_value_by", valueIncrement) != 0;

  if (autoGenerate)
  {
    for (int step = 0; step < numberOfSteps; ++step)
    {
      auto& info = timeSteps[step];
      info.GeometryIndex = index;
      info.FieldIndex = index;
      info.Value = value;
      index += indexIncrement;
      value += valueIncrement;
    }
  }

  for (int i = 0; i < element->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* nested = element->GetNestedElement(i);
    int stepIndex;
    if (strcmp(nested->GetName(), "TimeStep") != 0 ||
      !nested->GetScalarAttribute("index", stepIndex))
    {
      continue;
    }
    numberOfSteps = std::max(numberOfSteps, stepIndex + 1);
    auto& info = timeSteps[stepIndex];
    nested->GetScalarAttribute("geometry_index", info.GeometryIndex);
    nested->GetScalarAttribute("field_index", info.FieldIndex);
    if (nested->GetScalarAttribute("value", info.Value))
    {
      hasValues = true;
    }
  }
}
}

//-----------------------------------------------------------------------------
bool vtkGmshMetaFileReader::ParseMetaFile()
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified.");
    return false;
  }

  vtkSmartPointer<vtkXMLDataElement> root;
  root.TakeReference(vtkXMLUtilities::ReadElementFromFile(this->FileName));
  if (!root || strcmp(root->GetName(), "GmshMetaFile") != 0)
  {
    vtkErrorMacro("Cannot read " << this->FileName << " as a Gmsh meta-file.");
    return false;
  }

  vtkGmshMetaFileReaderInternals& internals = *this->Internals;
  internals = vtkGmshMetaFileReaderInternals();
  if (!root->GetScalarAttribute("number_of_partitioned_msh_files", internals.NumberOfFiles) &&
    !root->GetScalarAttribute("number_of_files", internals.NumberOfFiles))
  {
    internals.NumberOfFiles = 1;
  }

  this->AdaptationLevelInfo = 0;
  this->AdaptationToleranceInfo = -0.0001;
  int numberOfSteps = 1;
  bool hasValues = false;
  for (int i = 0; i < root->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* nested = root->GetNestedElement(i);
    const char* name = nested->GetName();
    if (strcmp(name, "GeometryInfo") == 0 && nested->GetAttribute("path"))
    {
      internals.GeometryPattern = nested->GetAttribute("path");
    }
    else if (strcmp(name, "FieldInfo") == 0)
    {
      for (int j = 0; j < nested->GetNumberOfNestedElements(); ++j)
      {
        vtkXMLDataElement* pathList = nested->GetNestedElement(j);
        const char* pattern = pathList->GetAttribute("path");
        if (strcmp(pathList->GetName(), "PathList") != 0 || !pattern)
        {
          continue;
        }
        const std::string arrayName = ::GetArrayName(pattern);
        internals.Fields.push_back({ pattern, arrayName });
        if (!this->PointDataArraySelection->ArrayExists(arrayName.c_str()))
        {
          this->PointDataArraySelection->AddArray(arrayName.c_str());
        }
      }
    }
    else if (strcmp(name, "TimeSteps") == 0)
    {
      ::ParseTimeSteps(nested, internals.TimeSteps, numberOfSteps, hasValues);
    }
    else if (strcmp(name, "AdaptView") == 0)
    {
      nested->GetScalarAttribute("adaptation_level", this->AdaptationLevelInfo);
      nested->GetScalarAttribute("adaptation_tol", this->AdaptationToleranceInfo);
    }
  }

  if (internals.GeometryPattern.empty())
  {
    vtkErrorMacro("No geometry path (GeometryInfo) in " << this->FileName);
    return false;
  }
  for (int step = 0; step < numberOfSteps; ++step)
  {
    // Without a TimeSteps element, the only step uses index -1 in the patterns.
    if (step > 0 && internals.TimeSteps.find(step) == internals.TimeSteps.end())
    {
      vtkErrorMacro("Missing time step, index=" << step);
      return false;
    }
    if (hasValues)
    {
      internals.TimeValues.push_back(internals.TimeSteps[step].Value);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
int vtkGmshMetaFileReader::GetRequestedTimeStep(vtkInformation* outInfo) const
{
  const std::vector<double>& times = this->Internals->TimeValues;
  if (times.empty() || !outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    return 0;
  }
  const double time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  const auto closest = std::min_element(times.begin(), times.end(),
    [time](double a, double b) { return std::abs(a - time) < std::abs(b - time); });
  return static_cast<int>(closest - times.begin());
}

//-----------------------------------------------------------------------------
int vtkGmshMetaFileReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  auto output = vtkPartitionedDataSetCollection::GetData(outInfo);
  const int timeStep = this->GetRequestedTimeStep(outInfo);
  const int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  const int numberOfPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // Same structure on all ranks: one partition per file, filled where read.
  const int numberOfFiles = this->Internals->NumberOfFiles;
  output->SetNumberOfPartitionedDataSets(1);
  vtkNew<vtkPartitionedDataSet> partitions;
  partitions->SetNumberOfPartitions(numberOfFiles);
  output->SetPartitionedDataSet(0, partitions);

  for (int file = piece; file < numberOfFiles; file += numberOfPieces)
  {
    vtkNew<vtkUnstructuredGrid> grid;
    if (!this->ReadPiece(file, timeStep, grid))
    {
      return 0;
    }
    partitions->SetPartition(file, grid);
  }

  if (!this->Internals->TimeValues.empty())
  {
    output->GetInformation()->Set(
      vtkDataObject::DATA_TIME_STEP(), this->Internals->TimeValues[timeStep]);
  }
  return 1;
}

//-----------------------------------------------------------------------------
namespace
{
// Substitute the [partID], [zeroPadPartID] and [step] patterns and make the
// path absolute, relative paths being relative to the meta-file.
std::string GetFilePath(
  const std::string& pattern, int partId, int step, const std::string& metaFileDirectory)
{
  std::string path = pattern;
  vtksys::SystemTools::ReplaceString(path, "[partID]", vtk::to_string(partId));
  std::string padded = vtk::to_string(partId);
  padded.insert(0, padded.size() < 6 ? 6 - padded.size() : 0, '0');
  vtksys::SystemTools::ReplaceString(path, "[zeroPadPartID]", padded);
  vtksys::SystemTools::ReplaceString(path, "[step]", vtk::to_string(step));
  if (!vtksys::SystemTools::FileIsFullPath(path) && !metaFileDirectory.empty())
  {
    path = metaFileDirectory + "/" + path;
  }
  return vtksys::SystemTools::CollapseFullPath(path);
}

//-----------------------------------------------------------------------------
struct CellTypeInfo
{
  int VTKType;
  // VTK position of each Gmsh node, empty when the orders match.
  std::vector<int> Order;
};

// Gmsh element types (see gmsh::model::mesh::getElementProperties) and their
// VTK equivalent.
const std::unordered_map<int, CellTypeInfo> GMSH_TO_VTK_CELL = {
  { 15, { VTK_VERTEX, {} } },
  { 1, { VTK_LINE, {} } },
  { 2, { VTK_TRIANGLE, {} } },
  { 3, { VTK_QUAD, {} } },
  { 4, { VTK_TETRA, {} } },
  { 5, { VTK_HEXAHEDRON, {} } },
  { 6, { VTK_WEDGE, {} } },
  { 7, { VTK_PYRAMID, {} } },
  { 8, { VTK_QUADRATIC_EDGE, {} } },
  { 9, { VTK_QUADRATIC_TRIANGLE, {} } },
  { 10, { VTK_BIQUADRATIC_QUAD, {} } },
  { 11, { VTK_QUADRATIC_TETRA, { 0, 1, 2, 3, 4, 5, 6, 7, 9, 8 } } },
  { 16, { VTK_QUADRATIC_QUAD, {} } },
};

//-----------------------------------------------------------------------------
// The cells of the highest dimension among 3 and 2, with their nodes.
void ReadMesh(vtkUnstructuredGrid* grid)
{
  int dimension = 3;
  std::vector<int> elementTypes;
  gmsh::model::mesh::getElementTypes(elementTypes, dimension);
  if (elementTypes.empty())
  {
    dimension = 2;
    gmsh::model::mesh::getElementTypes(elementTypes, dimension);
  }

  std::vector<std::size_t> nodeTags;
  std::vector<double> coordinates, parametricCoordinates;
  gmsh::model::mesh::getNodes(nodeTags, coordinates, parametricCoordinates, -1, -1, true, false);
  std::unordered_map<std::size_t, std::size_t> nodeIndex;
  for (std::size_t i = 0; i < nodeTags.size(); ++i)
  {
    nodeIndex[nodeTags[i]] = i;
  }

  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  std::unordered_map<std::size_t, vtkIdType> pointIds;
  std::vector<vtkIdType> cellPointIds;
  for (int elementType : elementTypes)
  {
    const auto typeIt = GMSH_TO_VTK_CELL.find(elementType);
    if (typeIt == GMSH_TO_VTK_CELL.end())
    {
      vtkGenericWarningMacro("Unsupported Gmsh element type " << elementType);
      continue;
    }
    const CellTypeInfo& info = typeIt->second;
    std::vector<std::size_t> elementTags, elementNodeTags;
    gmsh::model::mesh::getElementsByType(elementType, elementTags, elementNodeTags);
    if (elementTags.empty())
    {
      continue;
    }
    const std::size_t numberOfNodes = elementNodeTags.size() / elementTags.size();
    cellPointIds.resize(numberOfNodes);
    for (std::size_t element = 0; element < elementTags.size(); ++element)
    {
      for (std::size_t node = 0; node < numberOfNodes; ++node)
      {
        const std::size_t tag = elementNodeTags[element * numberOfNodes + node];
        auto inserted = pointIds.emplace(tag, points->GetNumberOfPoints());
        if (inserted.second)
        {
          points->InsertNextPoint(&coordinates[3 * nodeIndex[tag]]);
        }
        cellPointIds[info.Order.empty() ? node : info.Order[node]] = inserted.first->second;
      }
      grid->InsertNextCell(
        info.VTKType, static_cast<vtkIdType>(numberOfNodes), cellPointIds.data());
    }
  }
  grid->SetPoints(points);
}
}

//-----------------------------------------------------------------------------
bool vtkGmshMetaFileReader::ReadPiece(int piece, int timeStep, vtkUnstructuredGrid* grid)
{
  const vtkGmshMetaFileReaderInternals& internals = *this->Internals;
  const auto stepIt = internals.TimeSteps.find(timeStep);
  const vtkGmshMetaFileReaderInternals::TimeStepInfo stepInfo = stepIt != internals.TimeSteps.end()
    ? stepIt->second
    : vtkGmshMetaFileReaderInternals::TimeStepInfo();
  const std::string directory = vtksys::SystemTools::GetFilenamePath(this->FileName);
  const int partId = piece + 1;
  const std::string geometryFile =
    ::GetFilePath(internals.GeometryPattern, partId, stepInfo.GeometryIndex, directory);
  if (!vtksys::SystemTools::FileExists(geometryFile, true))
  {
    vtkErrorMacro("Cannot find the geometry file " << geometryFile);
    return false;
  }

  std::vector<std::string> fieldFiles;
  for (const auto& field : internals.Fields)
  {
    if (!this->PointDataArraySelection->ArrayIsEnabled(field.ArrayName.c_str()))
    {
      continue;
    }
    fieldFiles.push_back(::GetFilePath(field.Pattern, partId, stepInfo.FieldIndex, directory));
    if (!vtksys::SystemTools::FileExists(fieldFiles.back(), true))
    {
      // Solvers may write field files only for the partitions that have data
      // (e.g. boundary output), so a missing file gives an empty partition.
      return true;
    }
  }

  bool success = true;
  gmsh::initialize(0, nullptr, false);
  try
  {
    gmsh::option::setNumber("General.Verbosity", 1);
    // Partitioned MSH2 files: keep the old-style partitions and ignore periodic
    // boundaries, which may refer to other partitions.
    gmsh::option::setNumber("Mesh.PartitionCreateTopology", 0);
    gmsh::option::setNumber("Mesh.IgnorePeriodicity", 1);
    gmsh::open(geometryFile);
    if (fieldFiles.empty())
    {
      ::ReadMesh(grid);
    }
    else
    {
      for (const std::string& fieldFile : fieldFiles)
      {
        std::vector<int> before, after;
        gmsh::view::getTags(before);
        gmsh::merge(fieldFile);
        gmsh::view::getTags(after);
        if (after.size() == before.size())
        {
          vtkWarningMacro("No field data was read from "
            << fieldFile << ". Gmsh skips MSH2 files whose section end tags are followed by "
            << "spaces, such as \"$EndMeshFormat \".");
        }
      }
      this->ReadAdaptedFields(grid);
    }
  }
  catch (const std::exception& e)
  {
    vtkErrorMacro("Gmsh failed to read " << geometryFile << ": " << e.what());
    success = false;
  }
  gmsh::clear();
  gmsh::finalize();
  return success;
}

//-----------------------------------------------------------------------------
namespace
{
struct ListCellInfo
{
  int VTKType;
  int NumberOfNodes;
};

// Gmsh list data element letters (second letter of "ST", "VQ", ...).
const std::unordered_map<char, ListCellInfo> LIST_CELL = {
  { 'P', { VTK_VERTEX, 1 } },
  { 'L', { VTK_LINE, 2 } },
  { 'T', { VTK_TRIANGLE, 3 } },
  { 'Q', { VTK_QUAD, 4 } },
  { 'S', { VTK_TETRA, 4 } },
  { 'H', { VTK_HEXAHEDRON, 8 } },
  { 'I', { VTK_WEDGE, 6 } },
  { 'Y', { VTK_PYRAMID, 5 } },
};

// Gmsh list data value letters (first letter): scalar, vector, tensor.
int GetNumberOfComponents(char letter)
{
  return letter == 'S' ? 1 : (letter == 'V' ? 3 : 9);
}

//-----------------------------------------------------------------------------
struct AdaptedView
{
  int Tag;
  std::vector<std::string> Types;
  std::vector<int> NumberOfElements;
  std::vector<std::vector<double>> Data;
};

// Refine a view of the current time step and get its linear cells.
AdaptedView AdaptView(int tag, int level, double tolerance)
{
  const std::string option = "View[" + vtk::to_string(gmsh::view::getIndex(tag)) + "].";
  // The view holds one time step, the one of the merged file.
  double numberOfSteps;
  gmsh::option::getNumber(option + "NbTimeStep", numberOfSteps);
  gmsh::option::setNumber(option + "TimeStep", numberOfSteps - 1);
  gmsh::option::setNumber(option + "MaxRecursionLevel", level);
  gmsh::option::setNumber(option + "TargetError", tolerance);
  gmsh::option::setNumber(option + "AdaptVisualizationGrid", 1);

  AdaptedView view;
  view.Tag = tag;
  gmsh::view::getListData(tag, view.Types, view.NumberOfElements, view.Data, true);
  return view;
}

//-----------------------------------------------------------------------------
void InsertCells(const AdaptedView& view, vtkPoints* points, vtkUnstructuredGrid* grid)
{
  std::vector<vtkIdType> cellPointIds;
  for (std::size_t type = 0; type < view.Types.size(); ++type)
  {
    const auto cellIt = LIST_CELL.find(view.Types[type][1]);
    if (cellIt == LIST_CELL.end() || view.NumberOfElements[type] == 0)
    {
      continue;
    }
    const int numberOfNodes = cellIt->second.NumberOfNodes;
    const std::vector<double>& data = view.Data[type];
    const std::size_t stride = data.size() / view.NumberOfElements[type];
    cellPointIds.resize(numberOfNodes);
    for (int element = 0; element < view.NumberOfElements[type]; ++element)
    {
      // x of all nodes, then y, then z, then the values.
      const double* xyz = &data[element * stride];
      for (int node = 0; node < numberOfNodes; ++node)
      {
        cellPointIds[node] = points->InsertNextPoint(
          xyz[node], xyz[numberOfNodes + node], xyz[2 * numberOfNodes + node]);
      }
      grid->InsertNextCell(cellIt->second.VTKType, numberOfNodes, cellPointIds.data());
    }
  }
}

//-----------------------------------------------------------------------------
void AppendValues(const AdaptedView& view, std::vector<double>& values, int& numberOfComponents)
{
  for (std::size_t type = 0; type < view.Types.size(); ++type)
  {
    const auto cellIt = LIST_CELL.find(view.Types[type][1]);
    if (cellIt == LIST_CELL.end() || view.NumberOfElements[type] == 0)
    {
      continue;
    }
    const int numberOfNodes = cellIt->second.NumberOfNodes;
    numberOfComponents = ::GetNumberOfComponents(view.Types[type][0]);
    const std::vector<double>& data = view.Data[type];
    const std::size_t stride = data.size() / view.NumberOfElements[type];
    for (int element = 0; element < view.NumberOfElements[type]; ++element)
    {
      // The adapted data holds the values of a single time step.
      const double* first = &data[element * stride + 3 * numberOfNodes];
      values.insert(values.end(), first, first + numberOfNodes * numberOfComponents);
    }
  }
}
}

//-----------------------------------------------------------------------------
void vtkGmshMetaFileReader::ReadAdaptedFields(vtkUnstructuredGrid* grid)
{
  // Group the views by field: "Pressure_p_4" and "Pressure_p_3" are the parts
  // of the Pressure field with different interpolation orders.
  std::vector<int> tags;
  gmsh::view::getTags(tags);
  std::map<std::string, std::vector<std::pair<std::string, int>>> fields;
  for (int tag : tags)
  {
    std::string name;
    gmsh::option::getString("View[" + vtk::to_string(gmsh::view::getIndex(tag)) + "].Name", name);
    const std::size_t suffix = name.find("_p_");
    fields[name.substr(0, suffix)].emplace_back(
      suffix == std::string::npos ? std::string() : name.substr(suffix), tag);
  }
  if (fields.empty())
  {
    return;
  }

  double tolerance = this->AdaptationTolerance;
  if (tolerance >= 0 && fields.size() > 1)
  {
    vtkWarningMacro("Using a negative adaptation tolerance, since the "
      << fields.size() << " arrays must be refined the same way.");
    tolerance = -0.0001;
  }

  // The cells come from the first field; all fields refine the same cells.
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  bool hasScalars = false, hasVectors = false, hasTensors = false;
  for (auto& field : fields)
  {
    std::sort(field.second.begin(), field.second.end());
    std::vector<double> values;
    int numberOfComponents = 1;
    for (const auto& part : field.second)
    {
      const AdaptedView view = ::AdaptView(part.second, this->AdaptationLevel, tolerance);
      if (&field == &*fields.begin())
      {
        ::InsertCells(view, points, grid);
      }
      ::AppendValues(view, values, numberOfComponents);
    }

    const vtkIdType numberOfTuples = static_cast<vtkIdType>(values.size()) / numberOfComponents;
    if (numberOfTuples != points->GetNumberOfPoints())
    {
      vtkWarningMacro("Skipping " << field.first << ": its refined cells differ from those of "
                                  << fields.begin()->first << ".");
      continue;
    }
    vtkNew<vtkDoubleArray> array;
    array->SetName(field.first.c_str());
    array->SetNumberOfComponents(numberOfComponents);
    array->SetNumberOfTuples(numberOfTuples);
    std::copy(values.begin(), values.end(), array->GetPointer(0));
    vtkPointData* pointData = grid->GetPointData();
    pointData->AddArray(array);
    if (numberOfComponents == 1 && !hasScalars)
    {
      pointData->SetActiveScalars(array->GetName());
      hasScalars = true;
    }
    else if (numberOfComponents == 3 && !hasVectors)
    {
      pointData->SetActiveVectors(array->GetName());
      hasVectors = true;
    }
    else if (numberOfComponents == 9 && !hasTensors)
    {
      pointData->SetActiveTensors(array->GetName());
      hasTensors = true;
    }
  }
  grid->SetPoints(points);
}

//-----------------------------------------------------------------------------
void vtkGmshMetaFileReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "AdaptationLevel: " << this->AdaptationLevel << endl;
  os << indent << "AdaptationTolerance: " << this->AdaptationTolerance << endl;
  os << indent << "AdaptationLevelInfo: " << this->AdaptationLevelInfo << endl;
  os << indent << "AdaptationToleranceInfo: " << this->AdaptationToleranceInfo << endl;
  os << indent << "PointDataArraySelection:" << endl;
  this->PointDataArraySelection->PrintSelf(os, indent.GetNextIndent());
}
