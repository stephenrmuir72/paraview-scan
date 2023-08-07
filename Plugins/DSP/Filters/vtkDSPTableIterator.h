// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDSPTableIterator
 *
 * TODO
 */

#ifndef vtkDSPTableIterator_h
#define vtkDSPTableIterator_h

#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkDSPIterator.h"

#include <memory>

class vtkTable;

class vtkDSPTableIterator: public vtkDSPIterator2
{
public:
  vtkTypeMacro(vtkDSPTableIterator, vtkDSPIterator2);
  static vtkDSPTableIterator* New();

  virtual bool Initialize(vtkDataObject* object) override;
  virtual void GoToFirstItem() override;
  virtual void GoToNextItem() override;
  virtual bool IsDoneWithTraversal() override;
  virtual vtkTable* GetTable() override;

protected:
  vtkDSPTableIterator() = default;
  ~vtkDSPTableIterator() override = default;

private:
  vtkDSPTableIterator(const vtkDSPTableIterator&) = delete;
  void operator=(const vtkDSPTableIterator&) = delete;

  vtkSmartPointer<vtkTable> Table;
  int CurrentIdx = 0; // No GetIndex method on mdArray ...

  std::vector<std::shared_ptr<Worker>> Workers;
};

#endif // vtkDSPTableIterator_h
