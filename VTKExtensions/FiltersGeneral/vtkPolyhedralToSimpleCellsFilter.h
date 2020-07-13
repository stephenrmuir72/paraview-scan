/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyhedralToSimpleCellsFilter.h

  Copyright (c) Menno Deij - van Rijswijk (MARIN)
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPolyhedralToSimpleCellsFilter
 * @brief   Converts polyhedral and polygonal cells to simple cells if possible
 *
 *
 * vtkPolyhedralToSimpleCellsFilter takes an unstructured grid comprised of
 * cells of any cell type and attempts to convert polyhedral cells to simple
 * cells if possible.
 */

#ifndef vtkPolyhedralToSimpleCellsFilter_h
#define vtkPolyhedralToSimpleCellsFilter_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkUnstructuredGridBase;
class vtkIdList;
class TestPolyhedralToSimpleCellsFilter; // for testing purposes

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPolyhedralToSimpleCellsFilter
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPolyhedralToSimpleCellsFilter* New();
  vtkTypeMacro(vtkPolyhedralToSimpleCellsFilter, vtkUnstructuredGridAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPolyhedralToSimpleCellsFilter();
  ~vtkPolyhedralToSimpleCellsFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPolyhedralToSimpleCellsFilter(const vtkPolyhedralToSimpleCellsFilter&) = delete;
  void operator=(const vtkPolyhedralToSimpleCellsFilter&) = delete;

  // forward declared test class is friend so that it can call
  // the two functions below without having to expose them as public
  friend class TestPolyhedralToSimpleCellsFilter;

  void InsertNextPolyhedralCell(vtkUnstructuredGridBase*, vtkIdList*) const;
  void InsertNextPolygonalCell(vtkUnstructuredGridBase*, vtkIdList*) const;
};

#endif
