// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArrayDispatch.h"
#include "vtkDSPTableIterator.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkTypeList.h"

namespace
{
//-----------------------------------------------------------------------------
class Worker
{
public:
  virtual void SetIndex(vtkIdType index) = 0;
  virtual bool IsDoneWithTraversal(vtkIdType index) = 0;
};

//-----------------------------------------------------------------------------
template<typename MDArrayT>
class TypedWorker : public Worker
{
public:
  TypedWorker(MDArrayT* typedArray) : Array(typedArray) {}

  virtual void SetIndex(vtkIdType index)
  {
    this->Array->SetIndex(index);
  }

  virtual bool IsDoneWithTraversal(vtkIdType index)
  {
    return (index >= this->Array->GetNumberOfArrays());
  }

private:
  MDArrayT* Array = nullptr;
};

//-----------------------------------------------------------------------------
struct WorkerCreator
{
  template<typename ArrayT>
  void operator()(ArrayT* array, std::shared_ptr<Worker>& worker)
  {
    worker = std::make_shared<TypedWorker<ArrayT>>(array);
  }
};

//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<Worker>> DispatchInitialize(vtkTable* table)
{
  using MDArrayList = vtkTypeList::Create<
    vtkMultiDimensionalArray<double>,
    vtkMultiDimensionalArray<float>,
    vtkMultiDimensionalArray<int>,
    vtkMultiDimensionalArray<unsigned char>,
    vtkMultiDimensionalArray<vtkIdType>>;
  using MDDispatcher = vtkArrayDispatch::DispatchByArray<MDArrayList>;

  WorkerCreator workerCreator;

  std::vector<std::shared_ptr<Worker>> workers;
  for (vtkIdType iArr = 0; iArr < table->GetNumberOfColumns(); ++iArr)
  {
    vtkDataArray* array = vtkDataArray::SafeDownCast(table->GetColumn(iArr));
    if(!array)
    {
      continue;
    }

    std::shared_ptr<Worker> typeErasedWorker;
    MDDispatcher::Execute(array, workerCreator, typeErasedWorker);
    if (typeErasedWorker)
    {
      workers.emplace_back(typeErasedWorker);
    }
  }

  return workers;
}
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDSPTableIterator);

//-----------------------------------------------------------------------------
bool vtkDSPTableIterator::Initialize(vtkDataObject* object)
{
  this->Table = vtkTable::SafeDownCast(object);
  if (!this->Table)
  {
    vtkErrorMacro("vtkDSPTableIterator should be initialized with a vtkTable.");
    return false;
  }

  this->Workers = ::DispatchInitialize(this->Table); // Keep ?
  return true;
}

//-----------------------------------------------------------------------------
void vtkDSPTableIterator::GoToFirstItem()
{
  this->CurrentIdx = 0;
  std::for_each(this->Workers.begin(), this->Workers.end(),
      [&](std::shared_ptr<::Worker>& worker) { worker->SetIndex(this->CurrentIdx); });
}

//-----------------------------------------------------------------------------
void vtkDSPTableIterator::GoToNextItem()
{
  this->CurrentIdx ++;
  std::for_each(this->Workers.begin(), this->Workers.end(),
      [&](std::shared_ptr<::Worker>& worker) { worker->SetIndex(this->CurrentIdx); });
}

//-----------------------------------------------------------------------------
bool vtkDSPTableIterator::IsDoneWithTraversal()
{
  if (!this->Workers.empty())
  {
    return this->Workers[0]->IsDoneWithTraversal(this->CurrentIdx);
  }
  return true;
}

//-----------------------------------------------------------------------------
vtkTable* vtkDSPTableIterator::GetTable()
{
  return this->Table;
}
