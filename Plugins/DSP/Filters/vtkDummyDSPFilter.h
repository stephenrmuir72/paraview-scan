// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDummyDSPFilter
 *
 * TODO
 */

#ifndef vtkDummyDSPFilter_h
#define vtkDummyDSPFilter_h

#include "vtkDSPFiltersPluginModule.h"
#include "vtkTableAlgorithm.h"

class VTKDSPFILTERSPLUGIN_EXPORT vtkDummyDSPFilter : public vtkTableAlgorithm
{
public:
  static vtkDummyDSPFilter* New();
  vtkTypeMacro(vtkDummyDSPFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkDummyDSPFilter() = default;
  ~vtkDummyDSPFilter() override = default;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkDummyDSPFilter(const vtkDummyDSPFilter&) = delete;
  void operator=(const vtkDummyDSPFilter&) = delete;
};

#endif // vtkDummyDSPFilter_h
