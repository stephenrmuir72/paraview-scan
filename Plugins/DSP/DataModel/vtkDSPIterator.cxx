// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPIterator.h"

#include "vtkDSPMultiBlockIterator.h"
#include "vtkDSPTableIterator.h"
#include "vtkDataObjectTree.h"
#include "vtkTable.h"

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDSPIterator> vtkDSPIterator::GetInstance(vtkDataObject* object)
{
  vtkDataObjectTree* tree = vtkDataObjectTree::SafeDownCast(object);
  vtkTable* table = vtkTable::SafeDownCast(object);

  if (tree)
  {
    auto instance = vtkSmartPointer<vtkDSPMultiBlockIterator>::New(tree);
    return instance;
  }
  else if (table)
  {
    auto instance = vtkSmartPointer<vtkDSPTableIterator>::New(table);
    return instance;
  }
  else
  {
    vtkWarningWithObjectMacro(nullptr,
      "Can't create iterator instance: "
      "input should be a vtkMultiBlockDataSet, a vtkPartitionedDataSetCollection, or a vtkTable.");
    return nullptr;
  }
}
