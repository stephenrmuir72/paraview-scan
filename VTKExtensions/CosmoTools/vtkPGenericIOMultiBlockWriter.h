// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPGenericIOMultiBlockWriter
 *
 * Writes GenericIO files from a vtkMultiBlockDataSet or a
 * vtkPartitionedDataSetCollection whose leaves are vtkUnstructuredGrid; each
 * leaf becomes one rank's data in the GenericIO file.
 */

#ifndef vtkPGenericIOMultiBlockWriter_h
#define vtkPGenericIOMultiBlockWriter_h

#include "vtkPVVTKExtensionsCosmoToolsModule.h" // for export macro
#include "vtkWriter.h"

class vtkDataObjectTree;
class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPGenericIOMultiBlockWriter : public vtkWriter
{
public:
  static vtkPGenericIOMultiBlockWriter* New();
  vtkTypeMacro(vtkPGenericIOMultiBlockWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkPGenericIOMultiBlockWriter();
  ~vtkPGenericIOMultiBlockWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  bool WriteDataAndReturn() override;

private:
  class vtkInternals;
  vtkInternals* Internals;
  char* FileName;
  vtkMultiProcessController* Controller;
  vtkPGenericIOMultiBlockWriter(const vtkPGenericIOMultiBlockWriter&) = delete;
  void operator=(const vtkPGenericIOMultiBlockWriter&) = delete;
};

#endif
