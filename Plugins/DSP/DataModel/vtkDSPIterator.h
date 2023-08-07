// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDSPIterator
 *
 * TODO
 */

#ifndef vtkDSPIterator_h
#define vtkDSPIterator_h

#include "vtkDataObject.h"
#include "vtkDSPMultiBlockIterator.h"

class vtkDSPIterator2: public vtkDataObject
{
public:
  vtkTypeMacro(vtkDSPIterator2, vtkDataObject);

  static vtkSmartPointer<vtkDSPIterator2> GetInstance(vtkDataObject* object)
  {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(object);
    vtkTable* table = vtkTable::SafeDownCast(object);
    if (mb)
    {
      cout << "Multiblock ! " << endl; // TODO : return New(mb)
      auto instance = vtkSmartPointer<vtkDSPMultiBlockIterator>::New();
      // instance->SetDataObject(object);
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

  virtual void GoToFirstItem() = 0;

  virtual void GoToNextItem() = 0;

  virtual void IsDoneWithTraversal() = 0;

  virtual vtkTable* GetTable() = 0;

  // TODO: avoid this
  void SetDataObject(vtkDataObject* object)
  {
    this->DataObject = object;
  }

protected:
  vtkDSPIterator2() = default;
  ~vtkDSPIterator2() override = default;

private:
  vtkDSPIterator2(const vtkDSPIterator2&) = delete;
  void operator=(const vtkDSPIterator2&) = delete;

  // TODO: avoid this
  vtkSmartPointer<vtkDataObject> DataObject;
};

#endif // vtkDSPIterator_h
