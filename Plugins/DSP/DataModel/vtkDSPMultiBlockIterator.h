// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDSPMultiBlockIterator
 *
 * TODO
 */

#ifndef vtkDSPMultiBlockIterator_h
#define vtkDSPMultiBlockIterator_h

#include "vtkDataObject.h"
#include "vtkDSPIterator2.h"

class vtkDSPMultiBlockIterator: public vtkDSPIterator2
{
public:
  vtkTypeMacro(vtkDSPMultiBlockIterator, vtkDSPIterator2);

  virtual void GoToFirstItem()
  {

  }

  virtual void GoToNextItem()
  {

  }

  virtual void IsDoneWithTraversal()
  {

  }

  virtual vtkTable* GetTable()
  {
    cout << "A" << endl;
    return nullptr;
  }

protected:
  vtkDSPMultiBlockIterator() = default;
  ~vtkDSPMultiBlockIterator() override = default;

private:
  vtkDSPMultiBlockIterator(const vtkDSPMultiBlockIterator&) = delete;
  void operator=(const vtkDSPMultiBlockIterator&) = delete;
};

#endif // vtkDSPMultiBlockIterator_h
