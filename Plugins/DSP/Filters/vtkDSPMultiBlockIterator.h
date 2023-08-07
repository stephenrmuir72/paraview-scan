// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDSPMultiBlockIterator
 *
 * TODO
 */

#ifndef vtkDSPMultiBlockIterator_h
#define vtkDSPMultiBlockIterator_h

#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkDSPIterator.h"

class vtkTable;

class vtkDSPMultiBlockIterator: public vtkDSPIterator2
{
public:
  vtkTypeMacro(vtkDSPMultiBlockIterator, vtkDSPIterator2);
  static vtkDSPMultiBlockIterator* New();

  virtual bool Initialize(vtkDataObject* object) override;
  virtual void GoToFirstItem() override;
  virtual void GoToNextItem() override;
  virtual bool IsDoneWithTraversal() override;
  virtual vtkTable* GetTable() override;

protected:
  vtkDSPMultiBlockIterator() = default;
  ~vtkDSPMultiBlockIterator() override = default;

private:
  vtkDSPMultiBlockIterator(const vtkDSPMultiBlockIterator&) = delete;
  void operator=(const vtkDSPMultiBlockIterator&) = delete;

  vtkSmartPointer<vtkCompositeDataIterator> Iterator;
};

#endif // vtkDSPMultiBlockIterator_h
