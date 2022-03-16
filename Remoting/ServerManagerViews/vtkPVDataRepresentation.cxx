/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataPipeline.h"
#include "vtkPVCoreApplication.h"
#include "vtkPVDataRepresentationPipeline.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPVView.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <map>

class vtkPVDataRepresentation::vtkInternals
{
public:
  vtkTypeUInt32 GlobalID{ 0 };
  vtkPVView* View{ nullptr }; //  FIXME: Use vtkWeakPtr<> when available
  vtkSmartPointer<vtkMultiProcessController> Controller;

  bool Initialized{ false };
  bool InitializedForDataProcessing{ false };

  int NumberOfRenderingPorts{ 1 };
  bool Visibility{ true };
  double UpdateTime{ 0.0 };
  bool HasTemporalPipeline{ false };
  vtkTimeStamp PipelineDataTime;
  std::string LogName;

  vtkNew<vtkInformationVector> InformationVector;
};

using vtkSDDP = vtkStreamingDemandDrivenPipeline;

//----------------------------------------------------------------------------
vtkPVDataRepresentation::vtkPVDataRepresentation()
  : Internals(new vtkPVDataRepresentation::vtkInternals())
{
  // ensure that representations have no outputs.
  this->SetNumberOfOutputPorts(0);

  // setup default executive
  auto executive = vtkPVDataRepresentationPipeline::New();
  this->SetExecutive(executive);
  executive->FastDelete();
}

//----------------------------------------------------------------------------
vtkPVDataRepresentation::~vtkPVDataRepresentation() = default;

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetGlobalID(vtkTypeUInt32 gid)
{
  auto& internals = (*this->Internals);
  internals.GlobalID = gid;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVDataRepresentation::GetGlobalID() const
{
  const auto& internals = (*this->Internals);
  return internals.GlobalID;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetController(vtkMultiProcessController* controller)
{
  auto& internals = (*this->Internals);
  internals.Controller = controller;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVDataRepresentation::GetController() const
{
  const auto& internals = (*this->Internals);
  return internals.Controller;
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::GetNumberOfRenderingPorts() const
{
  const auto& internals = (*this->Internals);
  return internals.NumberOfRenderingPorts;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetNumberOfRenderingPorts(int ports)
{
  auto& internals = (*this->Internals);
  internals.NumberOfRenderingPorts = ports;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetVisibility(bool value)
{
  auto& internals = (*this->Internals);
  internals.Visibility = value;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::GetVisibility() const
{
  const auto& internals = (*this->Internals);
  return internals.Visibility;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetUpdateTime(double time)
{
  auto& internals = (*this->Internals);
  if (internals.UpdateTime != time)
  {
    internals.UpdateTime = time;
    if (internals.HasTemporalPipeline)
    {
      // This flag indicates if the representation is for a temporal pipeline. If
      // true, then calling `SetUpdateTime` will result in calling
      // `this->Modified()` i.e. will cause the representation to update on
      // subsequent update request. Otherwise, the subsequent update request will be
      // a no-op (unless the representation ended up calling Modified due to
      // other changes i.e. change in input pipeline).
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
double vtkPVDataRepresentation::GetUpdateTime() const
{
  const auto& internals = (*this->Internals);
  return internals.UpdateTime;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::SetLogName(const std::string& name)
{
  auto& internals = (*this->Internals);
  internals.LogName = name;
}

//----------------------------------------------------------------------------
const std::string& vtkPVDataRepresentation::GetLogName() const
{
  const auto& internals = (*this->Internals);
  return internals.LogName;
}

//----------------------------------------------------------------------------
vtkTypeBool vtkPVDataRepresentation::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkSDDP::REQUEST_DATA()))
  {
    this->Internals->PipelineDataTime.Modified();
    return this->RequestData(request, inputVector, outputVector);
  }

  // set update extent
  if (request->Has(vtkSDDP::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  if (request->Has(vtkSDDP::REQUEST_UPDATE_TIME()))
  {
    this->RequestUpdateTime(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::ProcessViewRequest(vtkInformation* request)
{
  assert(this->GetExecutive()->IsA("vtkPVDataRepresentationPipeline"));
  assert(this->GetVisibility());

  const auto& internals = (*this->Internals);
  if (request->Has(vtkPVView::REQUEST_UPDATE_DATA()))
  {
    // We could have called this->RequestDataToRender(...) here directly, but we
    // don't since we want to let the VTK-pipeline determine if the
    // representation really needs to execute based on changes to the pipeline.
    auto tstamp = this->GetPipelineDataTime();

    if (request->Has(vtkPVView::UPDATE_TIME_STEP()))
    {
      this->SetUpdateTime(request->Get(vtkPVView::UPDATE_TIME_STEP()));
    }

    // This is only called on the data-service side and never on the rendering
    // services.
    this->Update();

    if (tstamp != this->GetPipelineDataTime())
    {
      // Since the representation's pipeline re-executed, pass geometries to the view.
      for (int cc = 0; cc < internals.NumberOfRenderingPorts; ++cc)
      {
        auto* info = internals.InformationVector->GetInformationObject(cc);
        if (info->Get(vtkAlgorithm::ABORTED()) == 0)
        {
          vtkPVView::SetPiece(this, cc, vtkDataObject::GetData(info));
        }
      }
    }

    return true;
  }

  if (request->Has(vtkPVView::REQUEST_UPDATE_DATA_LOD()))
  {
    // TODO:
    // this->RequestLODData(request, internals.InformationVector,
    //      internals.LODInformationVector);
  }

  if (request->Has(vtkPVView::REQUEST_UPDATE_RENDERING_INFORMATION()))
  {
    // TODO:
    // return this->RequestRenderingInformation(request);
  }

  if (request->Has(vtkPVView::REQUEST_PREPARE_FOR_RENDER()))
  {
    vtkLogScopeF(INFO, "REQUEST_PREPARE_FOR_RENDER(%s)", vtkLogIdentifier(this));
    internals.InformationVector->SetNumberOfInformationObjects(internals.NumberOfRenderingPorts);
    for (int cc = 0; cc < internals.NumberOfRenderingPorts; ++cc)
    {
      auto* info = internals.InformationVector->GetInformationObject(cc);
      info->Set(vtkDataObject::DATA_OBJECT(), vtkPVView::GetPiece(this, cc));
    }
    return this->RequestPrepareForRender(request, internals.InformationVector);
  }

  if (request->Has(vtkPVView::REQUEST_PREPARE_FOR_RENDER_LOD()))
  {
    // TODO:
    // for (int cc=0; cc < internals.NumberOfRenderingPorts; ++cc)
    // {
    //   auto *info = internals.InformationVector->GetInformationObject(cc);
    //   info->Set(vtkDataObject::DATA_OBJECT(), vtkPVView::GetPiece(this, cc, /*lod=*/1));
    // }
    // return this->RequestPrepareForRenderLOD(request, internals.InformationVector);
  }

  if (request->Has(vtkPVView::REQUEST_RENDER()))
  {
    return this->RequestRender(request);
  }

  return false;
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  auto& internals = (*this->Internals);

  // Let's check if the upstream pipeline is temporal.
  // Need to check if'd still need this.
  internals.HasTemporalPipeline = false;
  for (int cc = 0; cc < this->GetNumberOfInputPorts(); cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      auto inInfo = inputVector[cc]->GetInformationObject(kk);
      internals.HasTemporalPipeline = internals.HasTemporalPipeline ||
        inInfo->Has(vtkSDDP::TIME_STEPS()) || inInfo->Has(vtkSDDP::TIME_RANGE());
    }
  }

  internals.InformationVector->SetNumberOfInformationObjects(internals.NumberOfRenderingPorts);
  for (int cc = 0; cc < internals.NumberOfRenderingPorts; ++cc)
  {
    internals.InformationVector->GetInformationObject(cc)->Clear();
  }

  return this->RequestDataToRender(inputVector, internals.InformationVector) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::RequestUpdateTime(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  const auto& internals = (*this->Internals);
  for (int cc = 0; cc < this->GetNumberOfInputPorts(); cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      inputVector[cc]->GetInformationObject(kk)->Set(
        vtkSDDP::UPDATE_TIME_STEP(), internals.UpdateTime);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVDataRepresentation::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // ideally, extent and time information will come from the view in
  // REQUEST_UPDATE(), include view-time.
  const auto* pvapp = vtkPVCoreApplication::GetInstance();
  const auto& internals = (*this->Internals);
  for (int cc = 0; cc < this->GetNumberOfInputPorts() && pvapp != nullptr; cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      vtkInformation* info = inputVector[cc]->GetInformationObject(kk);
      info->Set(vtkSDDP::UPDATE_PIECE_NUMBER(), pvapp->GetRank());
      info->Set(vtkSDDP::UPDATE_NUMBER_OF_PIECES(), pvapp->GetNumberOfRanks());
      info->Set(vtkSDDP::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
      info->Set(vtkSDDP::EXACT_EXTENT(), 1);
      info->Set(vtkSDDP::UPDATE_TIME_STEP(), internals.UpdateTime);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::RequestDataToRender(
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  const int numPorts = this->GetNumberOfInputPorts();
  for (int cc = 0; cc < numPorts && cc < this->GetNumberOfRenderingPorts(); ++cc)
  {
    const int numConnections = inputVector[cc]->GetNumberOfInformationObjects();
    if (numConnections > 0)
    {
      outputVector->GetInformationObject(cc)->Copy(inputVector[cc]->GetInformationObject(cc));
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::RequestPrepareForRender(
  vtkInformation* vtkNotUsed(request), vtkInformationVector* inputVector)
{
  assert(inputVector->GetNumberOfInformationObjects() == this->GetNumberOfRenderingPorts());
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::RequestRender(vtkInformation* vtkNotUsed(request))
{
  return true;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::InitializeForDataProcessing()
{
  auto& internals = (*this->Internals);
  internals.Initialized = true;
  internals.InitializedForDataProcessing = true;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::InitializeForRendering()
{
  auto& internals = (*this->Internals);
  internals.Initialized = true;
  internals.InitializedForDataProcessing = false;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::AddToView(vtkPVView* view)
{
  auto& internals = (*this->Internals);
  if (internals.View != nullptr)
  {
    vtkErrorMacro("Added representation has a non-null 'View'. "
                  "A representation cannot be added to two views at the same time!");
    return false;
  }
  internals.View = view;
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::RemoveFromView(vtkPVView* view)
{
  auto& internals = (*this->Internals);
  if (internals.View == view)
  {
    internals.View = nullptr;
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
vtkPVView* vtkPVDataRepresentation::GetView() const
{
  const auto& internals = (*this->Internals);
  return internals.View;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPVDataRepresentation::GetPipelineDataTime() const
{
  const auto& internals = (*this->Internals);
  return internals.PipelineDataTime;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::IsInitializedForDataProcessing() const
{
  const auto& internals = (*this->Internals);
  return internals.Initialized && internals.InitializedForDataProcessing;
}

//----------------------------------------------------------------------------
bool vtkPVDataRepresentation::IsInitializedForRendering() const
{
  const auto& internals = (*this->Internals);
  return internals.Initialized && !internals.InitializedForDataProcessing;
}

//----------------------------------------------------------------------------
void vtkPVDataRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
