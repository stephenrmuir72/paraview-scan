// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBivariateTextureRepresentation.h"

#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataObjectTypes.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkPVTrivialProducer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkPolyData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTree.h"

#include <algorithm>

vtkStandardNewMacro(vtkBivariateTextureRepresentation);

//----------------------------------------------------------------------------
vtkBivariateTextureRepresentation::vtkBivariateTextureRepresentation() = default;

//----------------------------------------------------------------------------
vtkBivariateTextureRepresentation::~vtkBivariateTextureRepresentation() = default;

//----------------------------------------------------------------------------
unsigned int vtkBivariateTextureRepresentation::Initialize(
  unsigned int minIdAvailable, unsigned int maxIdAvailable)
{
  unsigned int minId = minIdAvailable;
  if (this->LogoSourceRepresentation)
  {
    minId = this->LogoSourceRepresentation->Initialize(minId, maxIdAvailable);
  }
  return this->Superclass::Initialize(minId, maxIdAvailable);
}

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::SetVisibility(bool visible)
{
  if (this->LogoSourceRepresentation)
  {
    this->LogoSourceRepresentation->SetVisibility(visible);
  }
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------------
bool vtkBivariateTextureRepresentation::AddToView(vtkView* view)
{
  if (!this->Superclass::AddToView(view))
  {
    return false;
  }
  if (this->LogoSourceRepresentation)
  {
    // view->AddRepresentation(this->LogoSourceRepresentation);
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkBivariateTextureRepresentation::RemoveFromView(vtkView* view)
{
  if (this->LogoSourceRepresentation)
  {
    // view->RemoveRepresentation(this->LogoSourceRepresentation);
  }
  if (!this->Superclass::RemoveFromView(view))
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::SetTexture(vtkTexture* texture)
{
  this->LogoSource->SetTexture(texture);
  this->Superclass::SetTexture(texture);
  this->MarkModified(); // To force a new RequestData pass
}

//----------------------------------------------------------------------------
int vtkBivariateTextureRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Don't keep tcoords if all conditions are not met
  auto* out = vtkDataObjectTree::SafeDownCast(this->GetRenderedDataObject(0));
  if (out)
  {
    auto iter = vtk::TakeSmartPointer(out->NewTreeIterator());
    iter->SkipEmptyNodesOn();
    iter->VisitOnlyLeavesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      iter->GetCurrentDataObject()->GetAttributes(vtkDataObject::POINT)->SetTCoords(nullptr);
    }
  }

  if (this->LogoSourceRepresentation)
  {
    // Setup the internal pipeline
    this->LogoSourceRepresentation->SetInputConnection(this->LogoSource->GetOutputPort());
    this->LogoSourceRepresentation->Update();
  }

  // Retrieve input array names
  vtkInformation* info = this->GetInputArrayInformation(0);
  std::string arrayName1 =
    info->Has(vtkDataObject::FIELD_NAME()) ? info->Get(vtkDataObject::FIELD_NAME()) : "";

  info = this->GetInputArrayInformation(1);
  std::string arrayName2 =
    info->Has(vtkDataObject::FIELD_NAME()) ? info->Get(vtkDataObject::FIELD_NAME()) : "";

  if (!arrayName1.empty() && !arrayName2.empty())
  {
    // XXX: add support to MB
    vtkDataSet* inputDS = vtkDataSet::GetData(inputVector[0], 0);
    if (!inputDS)
    {
      // This can happen in client/server mode : RequestData is called on both
      // processes but data is only retrieved on server.
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

    // Retrieve input arrays
    auto* array1 =
      vtkDataArray::SafeDownCast(inputDS->GetPointData()->GetAbstractArray(arrayName1.c_str()));
    auto* array2 =
      vtkDataArray::SafeDownCast(inputDS->GetPointData()->GetAbstractArray(arrayName2.c_str()));

    if (!array1 || !array2)
    {
      // info->Get(vtkDataObject::FIELD_NAME()) can return non-empty
      // array names even if no array is available (e.g. "1 0 0 0 0")
      // In such case, we just call superclass RequestData
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

    if (array1->GetNumberOfComponents() != 1 || array2->GetNumberOfComponents() != 1)
    {
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

    // Compute texture coordinates from input array
    this->TCoordsArray->Initialize();
    this->TCoordsArray->SetName("BivariateTCoords");
    this->TCoordsArray->SetNumberOfComponents(2);
    this->TCoordsArray->SetNumberOfTuples(array1->GetNumberOfTuples());

    // Update arrays name & ranges
    auto* range1 = array1->GetRange();
    this->FirstArrayRange[0] = range1[0];
    this->FirstArrayRange[1] = range1[1];
    this->FirstArrayName = arrayName1;

    auto* range2 = array2->GetRange();
    this->SecondArrayRange[0] = range2[0];
    this->SecondArrayRange[1] = range2[1];
    this->SecondArrayName = arrayName2;

    // this->Modified(); // ?

    for (int i = 0; i < this->TCoordsArray->GetNumberOfTuples(); i++)
    {
      auto firstValue = (array1->GetTuple1(i) - range1[0]) / (range1[1] - range1[0]);
      auto secondValue = (array2->GetTuple1(i) - range2[0]) / (range2[1] - range2[0]);
      this->TCoordsArray->SetTuple2(i, firstValue, secondValue);
    }

    // Modify renderer DO TCoords
    out = vtkDataObjectTree::SafeDownCast(this->GetRenderedDataObject(0));
    if (out)
    {
      auto iter = vtk::TakeSmartPointer(out->NewTreeIterator());
      iter->SkipEmptyNodesOn();
      iter->VisitOnlyLeavesOn();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        iter->GetCurrentDataObject()->GetAttributes(vtkDataObject::POINT)->SetTCoords(this->TCoordsArray);
      }
    }
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::UpdateColoringParameters()
{
  // Do not color by color array
}

// //----------------------------------------------------------------------------
// bool vtkBivariateTextureRepresentation::NeedsOrderedCompositing()
// {
//   return false;
// }

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* attributeTypeorName)
{
  if ((idx == 0 || idx == 1) &&
    fieldAssociation == vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS)
  {
    this->MarkModified(); // To force a new RequestData pass
  }
  this->Superclass::SetInputArrayToProcess(
    idx, port, connection, fieldAssociation, attributeTypeorName);
}

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->LogoSource)
  {
    os << indent << "LogoSource:" << endl;
    this->LogoSource->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "LogoSource: (None)" << endl;
  }

  if (this->LogoSourceRepresentation)
  {
    os << indent << "LogoSourceRepresentation:" << endl;
    this->LogoSourceRepresentation->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "LogoSourceRepresentation: (None)" << endl;
  }

  if (this->TCoordsArray)
  {
    os << indent << "TCoordsArray:" << endl;
    this->TCoordsArray->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "TCoordsArray: (None)" << endl;
  }
}
