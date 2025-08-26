// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVExtractCellsByType.h"

#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkCellTypes.h>
#include <vtkCommand.h>
#include <vtkDataArraySelection.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include <vtkLogger.h>

#include <map>
#include <string>

vtkStandardNewMacro(vtkPVExtractCellsByType);

namespace
{
const std::map<unsigned char, std::string> SupportedCellTypes = { { VTK_VERTEX, "Vertex" },
  { VTK_POLY_VERTEX, "Polyvertex" }, { VTK_LINE, "Line" }, { VTK_POLY_LINE, "Polyline" },
  { VTK_TRIANGLE, "Triangle" }, { VTK_TRIANGLE_STRIP, "Triangle Strip" },
  { VTK_POLYGON, "Polygon" }, { VTK_PIXEL, "Pixel" }, { VTK_QUAD, "Quadrilateral" },
  { VTK_TETRA, "Tetrahedron" }, { VTK_VOXEL, "Voxel" }, { VTK_HEXAHEDRON, "Hexahedron" },
  { VTK_WEDGE, "Wedge" }, { VTK_PYRAMID, "Pyramid" }, { VTK_PENTAGONAL_PRISM, "Pentagonal Prism" },
  { VTK_HEXAGONAL_PRISM, "Hexagonal Prism" }, { VTK_POLYHEDRON, "Polyhedron" },
  { VTK_QUADRATIC_EDGE, "Quadratic Edge" }, { VTK_QUADRATIC_TRIANGLE, "Quadratic Triangle" },
  { VTK_QUADRATIC_QUAD, "Quadratic Quadrilateral" }, { VTK_QUADRATIC_POLYGON, "Quadratic Polygon" },
  { VTK_QUADRATIC_TETRA, "Quadratic Tetrahedron" },
  { VTK_QUADRATIC_HEXAHEDRON, "Quadratic Hexahedron" }, { VTK_QUADRATIC_WEDGE, "Quadratic Wedge" },
  { VTK_QUADRATIC_PYRAMID, "Quadratic Pyramid" },
  { VTK_BIQUADRATIC_QUAD, "Bi-Quadratic Quadrilateral" },
  { VTK_TRIQUADRATIC_HEXAHEDRON, "Tri-Quadratic Hexahedron" },
  { VTK_TRIQUADRATIC_PYRAMID, "Tri-Quadratic Pyramid" },
  { VTK_QUADRATIC_LINEAR_QUAD, "Quadratic Linear Quadrilateral" },
  { VTK_QUADRATIC_LINEAR_WEDGE, "Quadratic Linear Wedge" },
  { VTK_BIQUADRATIC_QUADRATIC_WEDGE, "Bi-Quadratic Quadratic Wedge" },
  { VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, "Bi-Quadratic Quadratic Hexahedron" },
  { VTK_BIQUADRATIC_TRIANGLE, "Bi-Quadratic Triangle" }, { VTK_CUBIC_LINE, "Cubic Line" },
  { VTK_LAGRANGE_CURVE, "Lagrange Curve" }, { VTK_LAGRANGE_TRIANGLE, "Lagrange Triangle" },
  { VTK_LAGRANGE_QUADRILATERAL, "Lagrange Quadrilateral" },
  { VTK_LAGRANGE_TETRAHEDRON, "Lagrange Tetrahedron" },
  { VTK_LAGRANGE_HEXAHEDRON, "Lagrange Hexahedron" }, { VTK_LAGRANGE_WEDGE, "Lagrange Wedge" },
  { VTK_LAGRANGE_PYRAMID, "Lagrange Pyramid" }, { VTK_BEZIER_CURVE, "Bezier Curve" },
  { VTK_BEZIER_TRIANGLE, "Bezier Triangle" }, { VTK_BEZIER_QUADRILATERAL, "Bezier Quadrilateral" },
  { VTK_BEZIER_TETRAHEDRON, "Bezier Tetrahedron" }, { VTK_BEZIER_HEXAHEDRON, "Bezier Hexahedron" },
  { VTK_BEZIER_WEDGE, "Bezier Wedge" }, { VTK_BEZIER_PYRAMID, "Bezier Pyramid" } };

std::string CellTypeToString(unsigned char cellType)
{
  if (SupportedCellTypes.find(cellType) == SupportedCellTypes.end())
  {
    vtkLogF(ERROR, "Could not find cell type %u", cellType);
    return "unknown cell";
  }

  return SupportedCellTypes.at(cellType);
}
}

// ----------------------------------------------------------------------------
vtkPVExtractCellsByType::vtkPVExtractCellsByType()
{
  // Add observer for selection update
  this->CellTypeSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkPVExtractCellsByType::Modified);
}

//------------------------------------------------------------------------------
int vtkPVExtractCellsByType::RequestInformation(
  vtkInformation*, vtkInformationVector** inInfo, vtkInformationVector*)
{
  if (this->AvailableCellTypes.empty())
  {
    vtkDataSet* dataSet = vtkDataSet::GetData(inInfo[0]);
    this->UpdateSelectionArray(dataSet);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkPVExtractCellsByType::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* dataSet = vtkDataSet::GetData(inputVector[0]);
  this->UpdateSelectionArray(dataSet);

  // Update selection of cell types
  this->RemoveAllCellTypes();

  for (const auto& type : SupportedCellTypes)
  {
    if (this->CellTypeSelection->ArrayIsEnabled(type.second.c_str()))
    {
      this->AddCellType(type.first);
    }
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
void vtkPVExtractCellsByType::UpdateSelectionArray(vtkDataSet* dataSet)
{
  // Extract available cells
  vtkNew<vtkCellTypes> cellTypes;
  dataSet->GetCellTypes(cellTypes);

  for (unsigned char cellType : this->AvailableCellTypes)
  {
    if (!this->HasCellType(cellTypes, cellType))
    {
      std::string cellTypeName = ::CellTypeToString(cellType);
      this->CellTypeSelection->RemoveArrayByName(cellTypeName.c_str());
    }
  }

  this->AvailableCellTypes.clear();
  for (vtkIdType cellTypeId = 0; cellTypeId < cellTypes->GetNumberOfTypes(); cellTypeId++)
  {
    unsigned char cellType = cellTypes->GetCellType(cellTypeId);
    this->AvailableCellTypes.insert(cellType);
    std::string cellTypeName = ::CellTypeToString(cellType);

    if (!this->CellTypeSelection->ArrayExists(cellTypeName.c_str()))
    {
      this->CellTypeSelection->AddArray(cellTypeName.c_str(), false);
    }
  }
}

//------------------------------------------------------------------------------
bool vtkPVExtractCellsByType::HasCellType(vtkCellTypes* cellTypes, unsigned char cellType)
{
  for (vtkIdType cellTypeId = 0; cellTypeId < cellTypes->GetNumberOfTypes(); cellTypeId++)
  {
    if (cellTypes->GetCellType(cellTypeId) == cellType)
    {
      return true;
    }
  }
  return false;
}
