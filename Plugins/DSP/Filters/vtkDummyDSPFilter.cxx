// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDummyDSPFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTypeList.h"

namespace
{
//-----------------------------------------------------------------------------
struct SetIndexWorker
{
  // Set Index for vtkMultiDimensional arrays
  template <typename ArrayType>
  void operator()(ArrayType* inArray, int idx)
  {
    inArray->SetIndex(idx);
  }
};

//-----------------------------------------------------------------------------
struct IsDoneWorker
{
  // Check if vtkMultiDimensional array is done with traversal
  template <typename ArrayType>
  void operator()(ArrayType* inArray, int idx, int& isDoneWithTraversal)
  {
    isDoneWithTraversal = (idx >= inArray->GetNumberOfArrays());
  }
};

//-----------------------------------------------------------------------------
typedef vtkTypeList::Unique<                 //
  vtkTypeList::Create<                       //
    vtkMultiDimensionalArray<double>,        //
    vtkMultiDimensionalArray<float>,         //
    vtkMultiDimensionalArray<int>,           //
    vtkMultiDimensionalArray<unsigned char>, //
    vtkMultiDimensionalArray<vtkIdType>      //
    >>::Result MultiDimArrays;

//-----------------------------------------------------------------------------
typedef vtkArrayDispatch::DispatchByArray<::MultiDimArrays> Dispatcher;

//-----------------------------------------------------------------------------
class vtkMultiDimensionalTableIterator: public vtkCompositeDataIterator
{
public:
  static vtkMultiDimensionalTableIterator* New();
  vtkTypeMacro(vtkMultiDimensionalTableIterator, vtkCompositeDataIterator);

  void setTable(vtkTable* table)
  {
    this->Table = table;
  }

  void GoToFirstItem() override
  {
    ::SetIndexWorker indexWorker;

    for (int i = 0; i < this->Table->GetNumberOfColumns(); i++)
    {
      vtkDataArray* array = vtkDataArray::SafeDownCast(this->Table->GetColumn(i));
      if (!Dispatcher::Execute(array, indexWorker, this->CurrentIdx))
      {
        cout << "Not a multi dimensional array !" << endl;
        return;
      }
    }

    this->CurrentIdx = 0;
  }

  void GoToNextItem() override
  {
    ::SetIndexWorker indexWorker;

    for (int i = 0; i < this->Table->GetNumberOfColumns(); i++)
    {
      vtkDataArray* array = vtkDataArray::SafeDownCast(this->Table->GetColumn(i));
      if (!Dispatcher::Execute(array, indexWorker, this->CurrentIdx))
      {
        cout << "Not a multi dimensional array !" << endl;
        return;
      }
    }

    this->CurrentIdx ++;
  }

  int IsDoneWithTraversal() override
  {
    ::IsDoneWorker isDoneWorker;

    vtkDataArray* array = vtkDataArray::SafeDownCast(this->Table->GetColumn(0));
    int isDoneWithTraversal = 0;
    if (!Dispatcher::Execute(array, isDoneWorker, this->CurrentIdx, isDoneWithTraversal))
    {
      cout << "Not a multi dimensional array !" << endl;
    }
    return isDoneWithTraversal;
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
      mdtIt->setTable(table);
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

  vtkNew<vtkDSPIterator> dspIterator;
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
