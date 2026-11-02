// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVBagPlotMatrixRepresentation.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTree.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkTable.h"

#include <cstring>

vtkStandardNewMacro(vtkPVBagPlotMatrixRepresentation);

//----------------------------------------------------------------------------
void vtkPVBagPlotMatrixRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

namespace
{
//----------------------------------------------------------------------------
// Returns the data of the block with the given name of a vtkMultiBlockDataSet or the first
// partition of the partitioned dataset with the given name of a vtkPartitionedDataSetCollection.
vtkDataObject* GetBlockData(vtkDataObjectTree* tree, const char* name)
{
  for (unsigned int i = 0; i < tree->GetNumberOfChildren(); ++i)
  {
    vtkInformation* info = tree->GetChildMetaData(i);
    if (info && info->Has(vtkCompositeDataSet::NAME()) &&
      strcmp(info->Get(vtkCompositeDataSet::NAME()), name) == 0)
    {
      vtkDataObject* child = tree->GetChild(i);
      if (auto partitionedDataSet = vtkPartitionedDataSet::SafeDownCast(child))
      {
        return partitionedDataSet->GetPartitionAsDataObject(0);
      }
      return child;
    }
  }
  return nullptr;
}
}

//----------------------------------------------------------------------------
int vtkPVBagPlotMatrixRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int ret = this->Superclass::RequestData(request, inputVector, outputVector);

  vtkSmartPointer<vtkDataObjectTree> data = vtkDataObjectTree::GetData(inputVector[0], 0);
  vtkTable* thresholdTable =
    data ? vtkTable::SafeDownCast(GetBlockData(data, "Threshold Data")) : nullptr;
  if (thresholdTable)
  {
    if (thresholdTable->GetNumberOfColumns() == 1)
    {
      vtkDoubleArray* array = vtkDoubleArray::SafeDownCast(thresholdTable->GetColumn(0));
      if (array)
      {
        if (array->GetNumberOfTuples() >= 5)
        {
          this->ExtractedExplainedVariance = array->GetValue(4);
        }
        else
        {
          vtkWarningMacro("Unexpected number of tuples in threshold array,"
                          " explained Variance not extracted");
        }
      }
      else
      {
        vtkWarningMacro("Threshold array is not of expected type,"
                        " explained Variance not extracted");
      }
    }
    else
    {
      vtkWarningMacro("Threshold table does not have the expected number of columns,"
                      " explained Variance not extracted");
    }
  }
  else
  {
    vtkWarningMacro("Threshold table could not be recovered from the data,"
                    " explained Variance not extracted");
  }
  return ret;
}
