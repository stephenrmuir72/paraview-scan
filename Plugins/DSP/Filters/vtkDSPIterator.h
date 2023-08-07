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
#include "vtkSmartPointer.h" // for vtkSmartPointer

class vtkTable;

class vtkDSPIterator2: public vtkDataObject
{
public:
  vtkTypeMacro(vtkDSPIterator2, vtkDataObject);

  static vtkSmartPointer<vtkDSPIterator2> GetInstance(vtkDataObject* object);

  // TODO: avoid this
  virtual bool Initialize(vtkDataObject* object) = 0;

  virtual void GoToFirstItem() = 0;

  virtual void GoToNextItem() = 0;

  virtual bool IsDoneWithTraversal() = 0;

  virtual vtkTable* GetTable() = 0;

protected:
  vtkDSPIterator2() = default;
  ~vtkDSPIterator2() override = default;

  // TODO: avoid this
  vtkSmartPointer<vtkDataObject> DataObject;

private:
  vtkDSPIterator2(const vtkDSPIterator2&) = delete;
  void operator=(const vtkDSPIterator2&) = delete;
};

#endif // vtkDSPIterator_h
