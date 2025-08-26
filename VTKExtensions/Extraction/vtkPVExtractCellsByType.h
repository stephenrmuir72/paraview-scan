// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkPVExtractCellsByType_h
#define vtkPVExtractCellsByType_h

#include "vtkPVVTKExtensionsExtractionModule.h" // For export macro

#include <vtkExtractCellsByType.h>
#include <vtkNew.h> // for vtkNew

#include <set>

class vtkDataArraySelection;
class vtkCellTypes;

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkPVExtractCellsByType : public vtkExtractCellsByType
{
public:
  static vtkPVExtractCellsByType* New();
  vtkTypeMacro(vtkPVExtractCellsByType, vtkExtractCellsByType);

  /**
   * Get the current selection of cell types for extraction.
   */
  vtkGetNewMacro(CellTypeSelection, vtkDataArraySelection);

protected:
  vtkPVExtractCellsByType();
  ~vtkPVExtractCellsByType() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVExtractCellsByType(const vtkPVExtractCellsByType&) = delete;
  void operator=(const vtkPVExtractCellsByType&) = delete;

  /**
   * Updates the selection array
   * - Remove cell types that are not available anymore.
   * - Add new available cell types.
   */
  void UpdateSelectionArray(vtkDataSet* dataSet);

  /**
   * Return true if cellType is in cellTypes array parameter.
   */
  bool HasCellType(vtkCellTypes* cellTypes, unsigned char cellType);

  vtkNew<vtkDataArraySelection> CellTypeSelection;
  std::set<unsigned char> AvailableCellTypes;
};

#endif
