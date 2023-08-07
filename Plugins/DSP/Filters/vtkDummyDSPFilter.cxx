// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDummyDSPFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDSPIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTypeList.h"

#include <memory>

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

//-----------------------------------------------------------------------------
class vtkMultiDimensionalTableIterator: public vtkCompositeDataIterator
{
public:
  static vtkMultiDimensionalTableIterator* New();
  vtkTypeMacro(vtkMultiDimensionalTableIterator, vtkCompositeDataIterator);

  void SetTable(vtkTable* table)
  {
    this->Table = table;
    this->Workers = ::DispatchInitialize(table); // Keep ?
  }

  void GoToFirstItem() override
  {
    this->CurrentIdx = 0;
    std::for_each(this->Workers.begin(), this->Workers.end(),
        [&](std::shared_ptr<::Worker>& worker) { worker->SetIndex(this->CurrentIdx); });
  }

  void GoToNextItem() override
  {
    this->CurrentIdx ++;
    std::for_each(this->Workers.begin(), this->Workers.end(),
        [&](std::shared_ptr<::Worker>& worker) { worker->SetIndex(this->CurrentIdx); });
  }

  int IsDoneWithTraversal() override
  {
    if (!this->Workers.empty())
    {
      return this->Workers[0]->IsDoneWithTraversal(this->CurrentIdx);
    }
    return true;
  }

  vtkDataObject* GetCurrentDataObject() override
  {
    return this->Table;
  }

  vtkInformation* GetCurrentMetaData() override
  {}

  int HasCurrentMetaData() override
  {}

  unsigned int GetCurrentFlatIndex() override
  {}

protected:
  vtkMultiDimensionalTableIterator() = default;
  ~vtkMultiDimensionalTableIterator() override = default;

private:
  vtkMultiDimensionalTableIterator(const vtkMultiDimensionalTableIterator&) = delete;
  void operator=(const vtkMultiDimensionalTableIterator&) = delete;

  vtkSmartPointer<vtkTable> Table;
  std::vector<std::shared_ptr<Worker>> Workers;
  int CurrentIdx = 0; // No GetIndex method on mdArray ...
};
vtkStandardNewMacro(vtkMultiDimensionalTableIterator)

//-----------------------------------------------------------------------------
class vtkDSPIterator: public vtkDataObject
{
public:
  static vtkDSPIterator* New();
  vtkTypeMacro(vtkDSPIterator, vtkDataObject);

  bool Initialize(vtkDataObject* object)
  {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(object);
    vtkTable* table = vtkTable::SafeDownCast(object);
    if (mb)
    {
      cout << "Multiblock ! " << endl;
      this->Iterator.TakeReference(mb->NewIterator());
      this->Iterator->SkipEmptyNodesOn();
    }
    else if (table)
    {
      cout << "Table ! " << endl;
      this->Iterator = vtkSmartPointer<vtkMultiDimensionalTableIterator>::New();
      vtkMultiDimensionalTableIterator* mdtIt = vtkMultiDimensionalTableIterator::SafeDownCast(this->Iterator);
      if (!mdtIt)
      {
        std::cerr << "Can't cast iterator !" << endl;
        return false;
      }
      mdtIt->SetTable(table);
    }
    else
    {
      vtkErrorWithObjectMacro(nullptr, "Not a multiblock ot a table !");
      return false;
    }
    return true;
  }

  void GoToFirstItem()
  {
    this->Iterator->GoToFirstItem();
  }

  void GoToNextItem()
  {
    this->Iterator->GoToNextItem();
  }

  int IsDoneWithTraversal()
  {
    return this->Iterator->IsDoneWithTraversal();
  }

  vtkTable* GetTable()
  {
    vtkTable* table = vtkTable::SafeDownCast(this->Iterator->GetCurrentDataObject());
    if (!table)
    {
      vtkErrorWithObjectMacro(nullptr, "Not a table");
    }
    return table;
  }

protected:
  vtkDSPIterator() = default;
  ~vtkDSPIterator() override = default;

private:
  vtkDSPIterator(const vtkDSPIterator&) = delete;
  void operator=(const vtkDSPIterator&) = delete;

  vtkSmartPointer<vtkCompositeDataIterator> Iterator;
};
vtkStandardNewMacro(vtkDSPIterator)
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDummyDSPFilter);

//------------------------------------------------------------------------------
int vtkDummyDSPFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//------------------------------------------------------------------------------
int vtkDummyDSPFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  if (!input)
  {
    vtkErrorMacro("Unable to retrieve the input !");
    return 0;
  }

  // vtkNew<vtkDSPIterator> dspIterator;

  auto dspIterator = vtkDSPIterator2::GetInstance(input);
  if (!dspIterator)
  {
    vtkErrorMacro("Unable to generate iterator !");
    return 0;
  }
  if (!dspIterator->Initialize(input))
  {
    vtkErrorMacro("Unable to initialize the DSP iterator");
    return 0;
  }

  for (dspIterator->GoToFirstItem(); !dspIterator->IsDoneWithTraversal(); dspIterator->GoToNextItem())
  {
    vtkTable* table = dspIterator->GetTable();
    if (!table)
    {
      vtkErrorMacro("Unable to retrieve the table !");
      return 0;
    }

    vtkAbstractArray* abstractArray = table->GetColumn(0);
    if (!abstractArray)
    {
      vtkErrorMacro("Unable to retrieve abstract array !");
      return 0;
    }

    vtkDataArray* array = vtkDataArray::SafeDownCast(abstractArray);
    if (!array)
    {
      vtkErrorMacro("Unable to retrieve data array !");
      return 0;
    }

    if (array->GetNumberOfTuples() == 0)
    {
      vtkErrorMacro("No values in array !");
      return 0;
    }

    if (array->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Wrong number of components !");
      return 0;
    }

    std::cout << array->GetTuple1(0) << endl;
  }

  // TODO
  return 1;
}

//--------------------------------------- --------------------------------------
void vtkDummyDSPFilter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
