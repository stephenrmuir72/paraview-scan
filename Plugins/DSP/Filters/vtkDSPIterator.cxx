// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDSPIterator.h"
#include "vtkDSPMultiBlockIterator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkTable.h"

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDSPIterator2> vtkDSPIterator2::GetInstance(vtkDataObject* object)
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(object);
  vtkTable* table = vtkTable::SafeDownCast(object);
  if (mb)
  {
    cout << "Multiblock ! " << endl; // TODO : return New(mb)
    auto instance = vtkSmartPointer<vtkDSPMultiBlockIterator>::New();
    instance->Initialize(mb);
    return instance;
  }
  // else if (table)
  // {
  //   cout << "Table ! " << endl; // TODO : return New(table)
  //   auto instance = vtkSmartPointer<vtkDSPMultiBlockIterator>::New();
  //   // instance->SetDataObject(object);
  // }
  else
  {
    vtkWarningWithObjectMacro(nullptr, "Can't create iterator instance: "
      "input should be a vtkMuliBlockDataSet or a vtkTable.");
    return nullptr;
  }
}
