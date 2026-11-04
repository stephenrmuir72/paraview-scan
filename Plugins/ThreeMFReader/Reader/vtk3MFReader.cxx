// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtk3MFReader.h"

#include "vtk3MFZipArchive.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkValueFromString.h"

#include <vtk_pugixml.h>

#include <array>
#include <cctype>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
const char* const CoreNamespace = "http://schemas.microsoft.com/3dmanufacturing/core/2015/02";
const char* const MaterialNamespace =
  "http://schemas.microsoft.com/3dmanufacturing/material/2015/02";
const char* const ProductionNamespace =
  "http://schemas.microsoft.com/3dmanufacturing/production/2015/06";
const char* const RelationshipsNamespace =
  "http://schemas.openxmlformats.org/package/2006/relationships";
const char* const ModelRelationshipType =
  "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel";
const char* const DefaultRootModelPath = "3D/3dmodel.model";

// Guard against cyclic or absurdly deep component hierarchies.
constexpr int MaxComponentDepth = 64;

using Color = std::array<unsigned char, 4>;
const Color DefaultColor = { { 255, 255, 255, 255 } };

//------------------------------------------------------------------------------
// Number parsing, independent of the current locale.
template <typename T>
bool ParseNext(const char*& cursor, T& value)
{
  while (*cursor && std::isspace(static_cast<unsigned char>(*cursor)))
  {
    ++cursor;
  }
  if (*cursor == '+')
  {
    ++cursor;
  }
  const std::size_t consumed = vtkValueFromString(cursor, cursor + std::strlen(cursor), value);
  cursor += consumed;
  return consumed > 0;
}

template <typename T>
bool ParseAttribute(const pugi::xml_attribute& attribute, T& value)
{
  const char* cursor = attribute.value();
  return attribute && ParseNext(cursor, value);
}

//------------------------------------------------------------------------------
// Affine transform, stored as a 3x4 matrix acting on column vectors.
struct Transform
{
  double M[3][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 } };

  // 3MF stores "m00 m01 m02 m10 m11 m12 m20 m21 m22 m30 m31 m32" for row
  // vectors (p' = p * M), i.e. the transpose of our layout.
  bool Parse(const pugi::xml_attribute& attribute)
  {
    if (!attribute)
    {
      return true; // identity
    }
    double values[12];
    const char* cursor = attribute.value();
    for (double& value : values)
    {
      if (!ParseNext(cursor, value))
      {
        return false;
      }
    }
    for (int row = 0; row < 3; ++row)
    {
      for (int col = 0; col < 3; ++col)
      {
        this->M[row][col] = values[col * 3 + row];
      }
      this->M[row][3] = values[9 + row];
    }
    return true;
  }

  // Return this * child: apply `child` first, then this transform.
  Transform Compose(const Transform& child) const
  {
    Transform result;
    for (int row = 0; row < 3; ++row)
    {
      for (int col = 0; col < 4; ++col)
      {
        double value = (col == 3) ? this->M[row][3] : 0.0;
        for (int k = 0; k < 3; ++k)
        {
          value += this->M[row][k] * child.M[k][col];
        }
        result.M[row][col] = value;
      }
    }
    return result;
  }

  void Apply(const double in[3], double out[3]) const
  {
    for (int row = 0; row < 3; ++row)
    {
      out[row] = this->M[row][0] * in[0] + this->M[row][1] * in[1] + this->M[row][2] * in[2] +
        this->M[row][3];
    }
  }

  double Determinant() const
  {
    const auto& m = this->M;
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
      m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
      m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  }
};

//------------------------------------------------------------------------------
// In-memory representation of a 3MF model part.
struct PropertyGroup
{
  // Empty for property groups that do not define colors (textures, ...).
  std::vector<Color> Colors;
};

struct Component
{
  std::string Path; // empty: same model part as the referencing object
  int ObjectId = -1;
  Transform Matrix;
};

struct Object
{
  int Id = -1;
  std::string Name;
  int PropertyId = -1;
  int PropertyIndex = 0;
  std::vector<double> Points;
  std::vector<vtkIdType> Triangles;         // 3 point ids per triangle
  std::vector<int> TrianglePropertyIds;     // per triangle, -1 for the object one
  std::vector<int> TrianglePropertyIndices; // per triangle, -1 for the object one
  std::vector<Component> Components;
};

struct BuildItem
{
  std::string Path; // empty: root model part
  int ObjectId = -1;
  Transform Matrix;
};

struct Model
{
  std::string Unit = "millimeter";
  std::map<int, Object> Objects;
  std::map<int, PropertyGroup> PropertyGroups;
  std::vector<BuildItem> Build;
  std::vector<std::pair<std::string, std::string>> Metadata;
};

//------------------------------------------------------------------------------
// pugixml is not namespace aware: resolve prefixes declared on the root element.
class Namespaces
{
public:
  void Initialize(const pugi::xml_node& root)
  {
    for (const pugi::xml_attribute& attribute : root.attributes())
    {
      const char* name = attribute.name();
      if (std::strcmp(name, "xmlns") == 0)
      {
        this->Default = attribute.value();
      }
      else if (std::strncmp(name, "xmlns:", 6) == 0)
      {
        this->Prefixes[name + 6] = attribute.value();
      }
    }
  }

  std::string Resolve(const std::string& prefix) const
  {
    const auto it = this->Prefixes.find(prefix);
    return it == this->Prefixes.end() ? std::string() : it->second;
  }

  static const char* LocalName(const char* name)
  {
    const char* colon = std::strchr(name, ':');
    return colon ? colon + 1 : name;
  }

  // Return true if `node` is the element `local` of namespace `ns`.
  bool Is(const pugi::xml_node& node, const char* ns, const char* local) const
  {
    const char* name = node.name();
    const char* colon = std::strchr(name, ':');
    if (colon)
    {
      return std::strcmp(colon + 1, local) == 0 && this->Resolve(std::string(name, colon)) == ns;
    }
    return std::strcmp(name, local) == 0 && this->Default == ns;
  }

  // Return the attribute `local` of namespace `ns` (prefixed attribute).
  pugi::xml_attribute Attribute(const pugi::xml_node& node, const char* ns, const char* local) const
  {
    for (const pugi::xml_attribute& attribute : node.attributes())
    {
      const char* name = attribute.name();
      const char* colon = std::strchr(name, ':');
      if (colon && std::strcmp(colon + 1, local) == 0 &&
        this->Resolve(std::string(name, colon)) == ns)
      {
        return attribute;
      }
    }
    return pugi::xml_attribute();
  }

private:
  std::string Default;
  std::map<std::string, std::string> Prefixes;
};

bool ParseColor(const char* text, Color& color)
{
  // sRGB "#RRGGBB" or "#RRGGBBAA".
  const std::size_t length = std::strlen(text);
  if (text[0] != '#' || (length != 7 && length != 9))
  {
    return false;
  }
  color[3] = 255;
  for (std::size_t c = 0; c < (length - 1) / 2; ++c)
  {
    unsigned int value = 0;
    for (std::size_t i = 0; i < 2; ++i)
    {
      const char ch = text[1 + 2 * c + i];
      const int digit = std::isdigit(static_cast<unsigned char>(ch))
        ? ch - '0'
        : (std::isxdigit(static_cast<unsigned char>(ch))
              ? std::tolower(static_cast<unsigned char>(ch)) - 'a' + 10
              : -1);
      if (digit < 0)
      {
        return false;
      }
      value = value * 16 + static_cast<unsigned int>(digit);
    }
    color[c] = static_cast<unsigned char>(value);
  }
  return true;
}

double UnitToMillimeters(const std::string& unit)
{
  static const std::map<std::string, double> scales = { { "micron", 0.001 }, { "millimeter", 1.0 },
    { "centimeter", 10.0 }, { "inch", 25.4 }, { "foot", 304.8 }, { "meter", 1000.0 } };
  const auto it = scales.find(unit);
  return it == scales.end() ? 1.0 : it->second;
}

//------------------------------------------------------------------------------
// Output geometry for one build item.
struct MeshBuffers
{
  vtkNew<vtkPoints> Points;
  vtkNew<vtkCellArray> Polys;
  vtkNew<vtkIntArray> ObjectIds;
  vtkNew<vtkUnsignedCharArray> Colors;

  MeshBuffers()
  {
    this->Points->SetDataTypeToDouble();
    this->ObjectIds->SetName("ObjectId");
    this->Colors->SetName("Color");
    this->Colors->SetNumberOfComponents(4);
  }
};

//------------------------------------------------------------------------------
class PackageReader
{
public:
  bool Open(const std::string& fileName)
  {
    if (!this->Archive.Open(fileName, this->Error))
    {
      return false;
    }
    this->RootPath = this->FindRootModelPath();
    if (this->RootPath.empty())
    {
      this->Error = "No 3D model part found in the package.";
      return false;
    }
    return true;
  }

  const Model* GetModel(const std::string& path)
  {
    const std::string key = vtk3MFZipArchive::NormalizeName(path);
    auto it = this->Models.find(key);
    if (it != this->Models.end())
    {
      return it->second.get();
    }
    auto model = std::unique_ptr<Model>(new Model);
    if (!this->ParseModel(path, *model))
    {
      return nullptr;
    }
    return (this->Models[key] = std::move(model)).get();
  }

  // Append the geometry of object `objectId` of model part `path`, transformed
  // by `matrix`, to `out`.
  bool AppendObject(
    const std::string& path, int objectId, const Transform& matrix, int depth, MeshBuffers& out)
  {
    if (depth > MaxComponentDepth)
    {
      this->Error = "Component hierarchy is too deep or cyclic.";
      return false;
    }
    const Model* model = this->GetModel(path);
    if (!model)
    {
      return false;
    }
    const auto it = model->Objects.find(objectId);
    if (it == model->Objects.end())
    {
      this->Error = "Object " + std::to_string(objectId) + " not found in '" + path + "'.";
      return false;
    }
    const Object& object = it->second;

    if (!object.Triangles.empty())
    {
      const vtkIdType offset = out.Points->GetNumberOfPoints();
      const vtkIdType numberOfPoints = static_cast<vtkIdType>(object.Points.size() / 3);
      for (vtkIdType i = 0; i < numberOfPoints; ++i)
      {
        double p[3];
        matrix.Apply(&object.Points[3 * i], p);
        out.Points->InsertNextPoint(p);
      }

      // A mirroring transform reverses the triangle orientation: compensate.
      const bool flip = matrix.Determinant() < 0.0;
      const std::size_t numberOfTriangles = object.Triangles.size() / 3;
      for (std::size_t t = 0; t < numberOfTriangles; ++t)
      {
        const vtkIdType* tri = &object.Triangles[3 * t];
        const vtkIdType ids[3] = { offset + tri[0], offset + (flip ? tri[2] : tri[1]),
          offset + (flip ? tri[1] : tri[2]) };
        out.Polys->InsertNextCell(3, ids);
        out.ObjectIds->InsertNextValue(objectId);

        const int pid =
          object.TrianglePropertyIds[t] >= 0 ? object.TrianglePropertyIds[t] : object.PropertyId;
        const int pindex = object.TrianglePropertyIndices[t] >= 0
          ? object.TrianglePropertyIndices[t]
          : object.PropertyIndex;
        const Color* color = &DefaultColor;
        const auto group = model->PropertyGroups.find(pid);
        if (group != model->PropertyGroups.end() && pindex >= 0 &&
          static_cast<std::size_t>(pindex) < group->second.Colors.size())
        {
          color = &group->second.Colors[pindex];
          this->HasColors = true;
        }
        out.Colors->InsertNextTypedTuple(color->data());
      }
    }

    for (const Component& component : object.Components)
    {
      if (!this->AppendObject(component.Path.empty() ? path : component.Path, component.ObjectId,
            matrix.Compose(component.Matrix), depth + 1, out))
      {
        return false;
      }
    }
    return true;
  }

  vtk3MFZipArchive Archive;
  std::string RootPath;
  std::string Error;
  std::vector<std::string> Warnings;
  bool HasColors = false;

private:
  std::string FindRootModelPath() const
  {
    std::string data, error;
    if (this->Archive.ReadEntry("_rels/.rels", data, error))
    {
      pugi::xml_document doc;
      if (doc.load_buffer_inplace(&data[0], data.size()))
      {
        const pugi::xml_node root = doc.document_element();
        Namespaces ns;
        ns.Initialize(root);
        for (const pugi::xml_node& rel : root.children())
        {
          if (ns.Is(rel, RelationshipsNamespace, "Relationship") &&
            std::strcmp(rel.attribute("Type").value(), ModelRelationshipType) == 0)
          {
            const std::string target = rel.attribute("Target").value();
            if (this->Archive.HasEntry(target))
            {
              return target;
            }
          }
        }
      }
    }
    if (this->Archive.HasEntry(DefaultRootModelPath))
    {
      return DefaultRootModelPath;
    }
    for (const std::string& name : this->Archive.GetEntryNames())
    {
      const std::string normalized = vtk3MFZipArchive::NormalizeName(name);
      if (normalized.size() > 6 && normalized.compare(normalized.size() - 6, 6, ".model") == 0)
      {
        return name;
      }
    }
    return std::string();
  }

  bool ParseError(const std::string& path, const std::string& message)
  {
    this->Error = "In '" + path + "': " + message;
    return false;
  }

  bool ParseModel(const std::string& path, Model& model)
  {
    std::string data, error;
    if (!this->Archive.ReadEntry(path, data, error))
    {
      this->Error = error;
      return false;
    }
    pugi::xml_document doc;
    const pugi::xml_parse_result result = doc.load_buffer_inplace(&data[0], data.size());
    if (!result)
    {
      return this->ParseError(path, std::string("XML error: ") + result.description());
    }

    const pugi::xml_node root = doc.document_element();
    Namespaces ns;
    ns.Initialize(root);
    if (!ns.Is(root, CoreNamespace, "model"))
    {
      return this->ParseError(path, "not a 3MF model (missing core 'model' element).");
    }

    if (const pugi::xml_attribute unit = root.attribute("unit"))
    {
      model.Unit = unit.value();
    }

    // Warn about required extensions this reader does not understand.
    const std::string required = root.attribute("requiredextensions").value();
    std::size_t start = 0;
    while ((start = required.find_first_not_of(" \t\r\n", start)) != std::string::npos)
    {
      const std::size_t end = required.find_first_of(" \t\r\n", start);
      const std::string prefix = required.substr(start, end - start);
      const std::string uri = ns.Resolve(prefix);
      if (uri != MaterialNamespace && uri != ProductionNamespace)
      {
        this->Warnings.push_back("Model requires unsupported extension '" +
          (uri.empty() ? prefix : uri) + "'; the geometry may be incomplete.");
      }
      start = end;
    }

    for (const pugi::xml_node& child : root.children())
    {
      if (ns.Is(child, CoreNamespace, "metadata"))
      {
        model.Metadata.emplace_back(child.attribute("name").value(), child.text().get());
      }
      else if (ns.Is(child, CoreNamespace, "resources"))
      {
        if (!this->ParseResources(path, ns, child, model))
        {
          return false;
        }
      }
      else if (ns.Is(child, CoreNamespace, "build"))
      {
        for (const pugi::xml_node& item : child.children())
        {
          if (!ns.Is(item, CoreNamespace, "item"))
          {
            continue;
          }
          BuildItem buildItem;
          if (!ParseAttribute(item.attribute("objectid"), buildItem.ObjectId) ||
            !buildItem.Matrix.Parse(item.attribute("transform")))
          {
            return this->ParseError(path, "invalid build item.");
          }
          buildItem.Path = ns.Attribute(item, ProductionNamespace, "path").value();
          model.Build.push_back(buildItem);
        }
      }
    }
    return true;
  }

  bool ParseResources(
    const std::string& path, const Namespaces& ns, const pugi::xml_node& resources, Model& model)
  {
    for (const pugi::xml_node& resource : resources.children())
    {
      int id = -1;
      if (!ParseAttribute(resource.attribute("id"), id))
      {
        continue; // not a resource we can reference
      }

      if (ns.Is(resource, CoreNamespace, "object"))
      {
        Object& object = model.Objects[id];
        object.Id = id;
        if (!this->ParseObject(path, ns, resource, object))
        {
          return false;
        }
      }
      else if (ns.Is(resource, CoreNamespace, "basematerials"))
      {
        PropertyGroup& group = model.PropertyGroups[id];
        for (const pugi::xml_node& base : resource.children())
        {
          if (ns.Is(base, CoreNamespace, "base"))
          {
            Color color = DefaultColor;
            ParseColor(base.attribute("displaycolor").value(), color);
            group.Colors.push_back(color);
          }
        }
      }
      else if (ns.Is(resource, MaterialNamespace, "colorgroup"))
      {
        PropertyGroup& group = model.PropertyGroups[id];
        for (const pugi::xml_node& entry : resource.children())
        {
          if (ns.Is(entry, MaterialNamespace, "color"))
          {
            Color color = DefaultColor;
            ParseColor(entry.attribute("color").value(), color);
            group.Colors.push_back(color);
          }
        }
      }
      else
      {
        // Other property groups (textures, composites, ...) share the id space:
        // register them so references to them resolve to "no color".
        model.PropertyGroups[id];
      }
    }
    return true;
  }

  bool ParseObject(
    const std::string& path, const Namespaces& ns, const pugi::xml_node& node, Object& object)
  {
    object.Name = node.attribute("name").value();
    ParseAttribute(node.attribute("pid"), object.PropertyId);
    ParseAttribute(node.attribute("pindex"), object.PropertyIndex);

    for (const pugi::xml_node& child : node.children())
    {
      if (ns.Is(child, CoreNamespace, "mesh"))
      {
        if (!this->ParseMesh(path, ns, child, object))
        {
          return false;
        }
      }
      else if (ns.Is(child, CoreNamespace, "components"))
      {
        for (const pugi::xml_node& item : child.children())
        {
          if (!ns.Is(item, CoreNamespace, "component"))
          {
            continue;
          }
          Component component;
          if (!ParseAttribute(item.attribute("objectid"), component.ObjectId) ||
            !component.Matrix.Parse(item.attribute("transform")))
          {
            return this->ParseError(
              path, "invalid component in object " + std::to_string(object.Id) + ".");
          }
          component.Path = ns.Attribute(item, ProductionNamespace, "path").value();
          object.Components.push_back(component);
        }
      }
    }
    return true;
  }

  bool ParseMesh(
    const std::string& path, const Namespaces& ns, const pugi::xml_node& mesh, Object& object)
  {
    for (const pugi::xml_node& child : mesh.children())
    {
      if (ns.Is(child, CoreNamespace, "vertices"))
      {
        for (const pugi::xml_node& vertex : child.children())
        {
          if (std::strcmp(Namespaces::LocalName(vertex.name()), "vertex") != 0)
          {
            continue;
          }
          double p[3];
          if (!ParseAttribute(vertex.attribute("x"), p[0]) ||
            !ParseAttribute(vertex.attribute("y"), p[1]) ||
            !ParseAttribute(vertex.attribute("z"), p[2]))
          {
            return this->ParseError(
              path, "invalid vertex in object " + std::to_string(object.Id) + ".");
          }
          object.Points.insert(object.Points.end(), p, p + 3);
        }
      }
      else if (ns.Is(child, CoreNamespace, "triangles"))
      {
        for (const pugi::xml_node& triangle : child.children())
        {
          if (std::strcmp(Namespaces::LocalName(triangle.name()), "triangle") != 0)
          {
            continue;
          }
          long long v[3];
          if (!ParseAttribute(triangle.attribute("v1"), v[0]) ||
            !ParseAttribute(triangle.attribute("v2"), v[1]) ||
            !ParseAttribute(triangle.attribute("v3"), v[2]))
          {
            return this->ParseError(
              path, "invalid triangle in object " + std::to_string(object.Id) + ".");
          }
          object.Triangles.insert(object.Triangles.end(), v, v + 3);
          int pid = -1;
          int p1 = -1;
          ParseAttribute(triangle.attribute("pid"), pid);
          ParseAttribute(triangle.attribute("p1"), p1);
          object.TrianglePropertyIds.push_back(pid);
          object.TrianglePropertyIndices.push_back(p1);
        }
      }
    }

    const long long numberOfPoints = static_cast<long long>(object.Points.size() / 3);
    for (const vtkIdType id : object.Triangles)
    {
      if (id < 0 || id >= numberOfPoints)
      {
        return this->ParseError(path,
          "triangle references a missing vertex in object " + std::to_string(object.Id) + ".");
      }
    }
    return true;
  }

  std::map<std::string, std::unique_ptr<Model>> Models;
};
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtk3MFReader);

//------------------------------------------------------------------------------
vtk3MFReader::vtk3MFReader()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtk3MFReader::~vtk3MFReader() = default;

//------------------------------------------------------------------------------
void vtk3MFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << "\n";
  os << indent << "ConvertToMillimeters: " << this->ConvertToMillimeters << "\n";
}

//------------------------------------------------------------------------------
int vtk3MFReader::CanReadFile(const char* fileName)
{
  PackageReader package;
  return (fileName && package.Open(fileName)) ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtk3MFReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outputVector);
  if (this->FileName.empty())
  {
    vtkErrorMacro("A FileName must be specified.");
    return 0;
  }

  PackageReader package;
  if (!package.Open(this->FileName))
  {
    vtkErrorMacro("Cannot read 3MF file '" << this->FileName << "': " << package.Error);
    return 0;
  }
  const Model* root = package.GetModel(package.RootPath);
  if (!root)
  {
    vtkErrorMacro("Cannot read 3MF file '" << this->FileName << "': " << package.Error);
    return 0;
  }

  Transform unitScale;
  if (this->ConvertToMillimeters)
  {
    const double scale = UnitToMillimeters(root->Unit);
    for (int i = 0; i < 3; ++i)
    {
      unitScale.M[i][i] = scale;
    }
  }

  std::vector<vtkSmartPointer<vtkPolyData>> meshes;
  std::vector<std::string> names;
  std::map<std::string, int> nameCounts;
  for (const BuildItem& item : root->Build)
  {
    if (this->CheckAbort())
    {
      break;
    }
    MeshBuffers buffers;
    const std::string path = item.Path.empty() ? package.RootPath : item.Path;
    if (!package.AppendObject(path, item.ObjectId, unitScale.Compose(item.Matrix), 0, buffers))
    {
      vtkErrorMacro("Cannot read 3MF file '" << this->FileName << "': " << package.Error);
      return 0;
    }

    auto mesh = vtkSmartPointer<vtkPolyData>::New();
    mesh->SetPoints(buffers.Points);
    mesh->SetPolys(buffers.Polys);
    mesh->GetCellData()->AddArray(buffers.ObjectIds);
    mesh->GetCellData()->AddArray(buffers.Colors);
    meshes.push_back(mesh);

    // Name the partition after its object, made unique.
    const Model* model = package.GetModel(path);
    const auto object = model->Objects.find(item.ObjectId);
    std::string name = object->second.Name;
    if (name.empty())
    {
      name = "Object " + std::to_string(item.ObjectId);
    }
    const int count = ++nameCounts[name];
    if (count > 1)
    {
      name += " (" + std::to_string(count) + ")";
    }
    names.push_back(name);

    this->UpdateProgress(static_cast<double>(meshes.size()) / root->Build.size());
  }

  output->SetNumberOfPartitionedDataSets(static_cast<unsigned int>(meshes.size()));
  for (unsigned int i = 0; i < meshes.size(); ++i)
  {
    if (!package.HasColors)
    {
      meshes[i]->GetCellData()->RemoveArray("Color");
    }
    output->SetPartition(i, 0, meshes[i]);
    output->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), names[i].c_str());
  }

  vtkNew<vtkStringArray> unit;
  unit->SetName("Unit");
  unit->InsertNextValue(this->ConvertToMillimeters ? std::string("millimeter") : root->Unit);
  output->GetFieldData()->AddArray(unit);
  for (const auto& entry : root->Metadata)
  {
    if (entry.first.empty() || output->GetFieldData()->HasArray(entry.first.c_str()))
    {
      continue;
    }
    vtkNew<vtkStringArray> metadata;
    metadata->SetName(entry.first.c_str());
    metadata->InsertNextValue(entry.second);
    output->GetFieldData()->AddArray(metadata);
  }

  for (const std::string& warning : package.Warnings)
  {
    vtkWarningMacro(<< warning);
  }
  if (root->Build.empty())
  {
    vtkWarningMacro("3MF file '" << this->FileName << "' has no build items.");
  }
  return 1;
}
