/*=========================================================================

  Program:   ParaView
  Module:    vtkGeometryRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeometryRepresentation.h"

#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVCoreApplication.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

class vtkGeometryRepresentation::vtkDataProcessingInternals
{
public:
  // Data Processing
  vtkNew<vtkPVGeometryFilter> GeometryFilter;

  vtkDataProcessingInternals()
  {
    this->GeometryFilter->SetUseOutline(0);
    this->GeometryFilter->SetTriangulate(0);
    this->GeometryFilter->SetNonlinearSubdivisionLevel(1);
    this->GeometryFilter->SetPassThroughCellIds(1);
    this->GeometryFilter->SetPassThroughPointIds(1);
  }
};

class vtkGeometryRepresentation::vtkRenderingInternals
{
public:
  // Rendering
  vtkNew<vtkCompositePolyDataMapper2> Mapper;
  vtkNew<vtkProperty> Property;
  vtkNew<vtkPVLODActor> Actor;

  vtkRenderingInternals()
  {
    this->Actor->SetMapper(this->Mapper);
    this->Actor->SetProperty(this->Property);
  }
};
int vtkGeometryRepresentation::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines = 1;
int vtkGeometryRepresentation::DefaultMinimumGhostLevelsToRequestForStructuredPipelines = 0;
vtkStandardNewMacro(vtkGeometryRepresentation);
//----------------------------------------------------------------------------
vtkGeometryRepresentation::vtkGeometryRepresentation()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
  this->SetNumberOfRenderingPorts(1);
}

//----------------------------------------------------------------------------
vtkGeometryRepresentation::~vtkGeometryRepresentation() = default;

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  // ensure that the ghost-level information is setup correctly to avoid
  // internal faces for unstructured grids.
  for (int cc = 0; cc < this->GetNumberOfInputPorts(); cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      auto* inInfo = inputVector[cc]->GetInformationObject(kk);
      int ghostLevels =
        inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
      ghostLevels += vtkGeometryRepresentation::GetNumberOfGhostLevelsToRequest(inInfo);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
    }
  }

  return 1;
}
//----------------------------------------------------------------------------
void vtkGeometryRepresentation::InitializeForDataProcessing()
{
  this->Superclass::InitializeForDataProcessing();
  this->DInternals.reset(new vtkGeometryRepresentation::vtkDataProcessingInternals());
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::InitializeForRendering()
{
  this->Superclass::InitializeForRendering();
  this->RInternals.reset(new vtkGeometryRepresentation::vtkRenderingInternals());
  this->RInternals->Actor->SetVisibility(this->GetVisibility());
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::AddToView(vtkPVView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview && this->RInternals)
  {
    auto& rinternals = (*this->RInternals);
    rview->GetRenderer()->AddActor(rinternals.Actor);
  }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::RemoveFromView(vtkPVView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview && this->RInternals)
  {
    auto& rinternals = (*this->RInternals);
    rview->GetRenderer()->RemoveActor(rinternals.Actor);
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetVisibility(bool val)
{
  if (this->RInternals)
  {
    this->RInternals->Actor->SetVisibility(val);
  }
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::RequestDataToRender(
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  assert(this->DInternals);

  auto& dinternals = (*this->DInternals);
  auto* input = vtkDataObject::GetData(inputVector[0], 0);
  dinternals.GeometryFilter->SetInputDataObject(input);
  dinternals.GeometryFilter->Update();

  auto* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::DATA_OBJECT(), dinternals.GeometryFilter->GetOutputDataObject(0));
  return true;
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::RequestPrepareForRender(
  vtkInformation* vtkNotUsed(request), vtkInformationVector* inputVector)
{
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  auto* input = vtkDataObject::GetData(inputVector, 0);
  rinternals.Mapper->SetInputDataObject(input);

  return true;
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::RequestRender(vtkInformation* vtkNotUsed(request))
{
  return true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetDefaultMinimumGhostLevelsToRequestForStructuredPipelines(int val)
{
  vtkGeometryRepresentation::DefaultMinimumGhostLevelsToRequestForStructuredPipelines = val;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines()
{
  return vtkGeometryRepresentation::DefaultMinimumGhostLevelsToRequestForStructuredPipelines;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines(
  int val)
{
  vtkGeometryRepresentation::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines = val;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines()
{
  return vtkGeometryRepresentation::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::GetNumberOfGhostLevelsToRequest(vtkInformation* info)
{
  if (vtkPVCoreApplication::GetInstance()->GetNumberOfRanks() <= 1)
  {
    return 0;
  }

  vtkDataSet* ds = vtkDataSet::GetData(info);
  if (ds || vtkCompositeDataSet::GetData(info) != nullptr)
  {
    // Check if this is structured-pipeline, this includes unstructured pipelines
    // downstream from structured source e.g. Wavelet - > Clip.
    // To do that, we use a trick. If WHOLE_EXTENT() key us present, it must have
    // started as a structured dataset.
    // The is one exception to this for ExplicitStructuredGrid data, their
    // behavior is both structured and unstructured but regarding ghost cells
    // they have the behavior of an unstructured dataset.
    const bool is_structured = (info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) != 0) &&
      (!ds || (ds && !ds->IsA("vtkExplicitStructuredGrid")));
    return is_structured
      ? vtkGeometryRepresentation::GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines()
      : vtkGeometryRepresentation::GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines();
  }
  else
  {
    // the pipeline is for a non-dataset/composite-dataset filter. don't bother
    // asking for ghost levels.
    return 0;
  }
}
//----------------------------------------------------------------------------
void vtkGeometryRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
