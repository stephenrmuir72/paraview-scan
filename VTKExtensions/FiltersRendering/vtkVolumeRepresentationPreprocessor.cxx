/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRepresentationPreprocessor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVolumeRepresentationPreprocessor.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataToUnstructuredGridFilter.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkExtractBlock.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVolumeRepresentationPreprocessor);

//----------------------------------------------------------------------------
vtkVolumeRepresentationPreprocessor::vtkVolumeRepresentationPreprocessor()
  : TetrahedraOnly(0)
  , ExtractedBlockIndices({ 0U })
{
}

//----------------------------------------------------------------------------
vtkVolumeRepresentationPreprocessor::~vtkVolumeRepresentationPreprocessor()
{
}

//----------------------------------------------------------------------------
bool vtkVolumeRepresentationPreprocessor::AddExtractedBlockIndex(unsigned int index)
{
  return this->ExtractedBlockIndices.insert(index).second;
}

//----------------------------------------------------------------------------
bool vtkVolumeRepresentationPreprocessor::RemoveExtractedBlockIndex(unsigned int index)
{
  return this->ExtractedBlockIndices.erase(index) > 0;
}

//----------------------------------------------------------------------------
bool vtkVolumeRepresentationPreprocessor::RemoveAllExtractedBlockIndices()
{
  if (this->ExtractedBlockIndices.empty())
  {
    return false;
  }
  this->ExtractedBlockIndices.clear();
  return true;
}

//----------------------------------------------------------------------------
int vtkVolumeRepresentationPreprocessor::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (vtkDataObjectTree::SafeDownCast(inputDO))
  {
    // Output is vtkDataObjectTree
    if (vtkDataObjectTree::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = inputDO->NewInstance();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkDataSet::SafeDownCast(inputDO))
  {
    // Output is vtkUnstructuredGrid
    if (vtkUnstructuredGrid::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // Output is vtkMultiBlockDataSet
    if (vtkMultiBlockDataSet::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkVolumeRepresentationPreprocessor::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkSmartPointer<vtkDataObject> input =
    vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataObject* output = vtkDataObject::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (vtkCompositeDataSet::SafeDownCast(input))
  {
    this->ExtractDataSet(input, output);
    this->Tetrahedralize(output, output);
  }
  else
  {
    this->Tetrahedralize(input, output);
  }
  // FIXME: vtkUnstructuredGrid::RemoveGhostCells() is not applicable for
  // vtkCompositeDatasets, currently commented out.
  // output->RemoveGhostCells();
  return 1;
}

//----------------------------------------------------------------------------
/// Pushes input dataset through a vtkDataSetTriangleFilter and returns
/// the output.
void vtkVolumeRepresentationPreprocessor::Tetrahedralize(
  vtkDataObject* input, vtkDataObject* output)
{
  vtkNew<vtkDataSetTriangleFilter> tetrahedralizer;
  tetrahedralizer->SetInputData(input);
  tetrahedralizer->SetTetrahedraOnly(this->TetrahedraOnly);
  tetrahedralizer->Update();
  output->ShallowCopy(tetrahedralizer->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
/// Extracts a single block from a multiblock dataset and attempts to downcast
/// the extracted block to dataset before returning.
void vtkVolumeRepresentationPreprocessor::ExtractDataSet(
  vtkDataObject* input, vtkDataObject* output)
{
  vtkNew<vtkExtractBlock> extractor;
  for (auto& index : this->ExtractedBlockIndices)
  {
    extractor->AddIndex(index);
  }
  extractor->SetInputData(input);
  extractor->Update();
  output->ShallowCopy(extractor->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkVolumeRepresentationPreprocessor::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentationPreprocessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ExtractedBlockIndices: ";
  for (auto& i : this->ExtractedBlockIndices)
  {
    os << indent << i;
  }
  os << "\n";
  os << indent << "TetrahedraOnly: " << (this->TetrahedraOnly ? "On" : "Off") << "\n";
}
