// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDSPMultiBlockIterator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDSPMultiBlockIterator);

//-----------------------------------------------------------------------------
bool vtkDSPMultiBlockIterator::Initialize(vtkDataObject* object)
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(object);
  if (!mb)
  {
    vtkErrorMacro("vtkDSPMultiBlockIterator should be initialized with a vtkMultiBlockDataSet.");
    return false;
  }
  this->Iterator.TakeReference(mb->NewIterator());
  this->Iterator->SkipEmptyNodesOn();
  return true;
}

//-----------------------------------------------------------------------------
void vtkDSPMultiBlockIterator::GoToFirstItem()
{
  this->Iterator->GoToFirstItem();
}

//-----------------------------------------------------------------------------
void vtkDSPMultiBlockIterator::GoToNextItem()
{
  this->Iterator->GoToNextItem();
}

//-----------------------------------------------------------------------------
bool vtkDSPMultiBlockIterator::IsDoneWithTraversal()
{
  return this->Iterator->IsDoneWithTraversal();
}

//-----------------------------------------------------------------------------
vtkTable* vtkDSPMultiBlockIterator::GetTable()
{
  vtkTable* table = vtkTable::SafeDownCast(this->Iterator->GetCurrentDataObject());
  if (!table)
  {
    vtkErrorWithObjectMacro(nullptr, "Not a table");
  }
  return table;
}
