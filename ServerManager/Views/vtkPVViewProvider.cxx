/*=========================================================================

  Program:   ParaView
  Module:    vtkPVViewProvider.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVViewProvider.h"

#include "vtkChannelSubscription.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkObjectWrapper.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVLogger.h"
#include "vtkPVView.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"

#include <cassert>

class vtkPVViewProvider::vtkInternals
{
public:
  vtkPVViewProvider* Self;
  vtkSmartPointer<vtkObjectStore> ObjectStore;
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkSmartPointer<vtkChannelSubscription> DataChannelSubscription;
  bool DataProcessing{ false };

  std::map<vtkTypeUInt32, vtkTimeStamp> PushTimeStamps;
  vtkInternals(vtkPVViewProvider* self)
    : Self(self)
  {
  }
};

vtkStandardNewMacro(vtkPVViewProvider);
//----------------------------------------------------------------------------
vtkPVViewProvider::vtkPVViewProvider()
  : Internals(new vtkPVViewProvider::vtkInternals(this))
{
}

//----------------------------------------------------------------------------
vtkPVViewProvider::~vtkPVViewProvider() = default;

//----------------------------------------------------------------------------
void vtkPVViewProvider::SetObjectStore(vtkObjectStore* store)
{
  auto& internals = (*this->Internals);
  internals.ObjectStore = store;
  store->AddObserver(
    vtkObjectStore::RegisterObjectEvent, this, &vtkPVViewProvider::HandleObjectStoreEvent);
  store->AddObserver(
    vtkObjectStore::UnregisterObjectEvent, this, &vtkPVViewProvider::HandleObjectStoreEvent);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkObjectStore> vtkPVViewProvider::GetObjectStore() const
{
  const auto& internals = (*this->Internals);
  return internals.ObjectStore;
}

//----------------------------------------------------------------------------
void vtkPVViewProvider::HandleObjectStoreEvent(
  vtkObject* caller, unsigned long eventId, void* callData)
{
  // this is called on service's main thread.
  auto& internals = (*this->Internals);
  if (eventId == vtkObjectStore::RegisterObjectEvent)
  {
    auto* wrapper = vtkObjectWrapper::SafeDownCast(reinterpret_cast<vtkObject*>(callData));
    if (auto* view = wrapper ? vtkPVView::SafeDownCast(wrapper->GetVTKObject()) : nullptr)
    {
      view->SetGlobalID(internals.ObjectStore->GetID(wrapper));
      view->SetController(this->GetController());
      if (internals.DataProcessing)
      {
        vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "Initialize for data processing (%s)",
          vtkLogIdentifier(view));
        view->InitializeForDataProcessing();
        view->AddObserver(vtkCommand::UpdateEvent, this, &vtkPVViewProvider::PushViewData);
      }
      else
      {
        vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "Initialize for rendering (%s)",
          vtkLogIdentifier(view));
        view->InitializeForRendering();
        view->AddObserver(vtkCommand::RenderEvent, this, &vtkPVViewProvider::PushViewImage);
      }
      internals.PushTimeStamps[view->GetGlobalID()].Modified();
    }
    else if (auto* repr =
               wrapper ? vtkPVDataRepresentation::SafeDownCast(wrapper->GetVTKObject()) : nullptr)
    {
      repr->SetGlobalID(internals.ObjectStore->GetID(wrapper));
      repr->SetController(this->GetController());
      if (internals.DataProcessing)
      {
        vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "Initialize for data processing (%s)",
          vtkLogIdentifier(repr));
        repr->InitializeForDataProcessing();
      }
      else
      {
        vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "Initialize for rendering (%s)",
          vtkLogIdentifier(repr));
        repr->InitializeForRendering();
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVViewProvider::PushViewData(vtkObject* caller, unsigned long eventId, void* callData)
{
  auto& internals = (*this->Internals);
  assert("Only supported on data-processing services" && internals.DataProcessing);

  auto* view = vtkPVView::SafeDownCast(caller);
  auto& tstamp = internals.PushTimeStamps.at(view->GetGlobalID());

  auto* dmanager = view->GetDataDeliveryManager();
  auto jsonDataSets = dmanager->GetData(tstamp);
  tstamp.Modified();

  if (!jsonDataSets.is_null())
  {
    vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "Pushing rendering data");
    vtkJson json;
    json["gid"] = view->GetGlobalID();
    json["datasets"] = std::move(jsonDataSets);
    this->GetService()->Publish(
      vtkPVViewProvider::GetDataChannelName(), vtkPacket{ std::move(json) });
  }
}

//----------------------------------------------------------------------------
void vtkPVViewProvider::PushViewImage(vtkObject* caller, unsigned long eventId, void* callData)
{
  auto& internals = (*this->Internals);
  assert("Only supported on rendering services" && !internals.DataProcessing);
  auto* view = vtkPVView::SafeDownCast(caller);
  vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "Pushing rendered image");
  this->GetService()->Publish(vtkPVViewProvider::GetImageChannelName(), view->GetRenderedResult());
}

//----------------------------------------------------------------------------
void vtkPVViewProvider::InitializeInternal(vtkService* service)
{
  auto& internals = (*this->Internals);
  internals.DataProcessing = (service->GetName() == "ds"); // TODO: need a better way
  if (!internals.DataProcessing)
  {
    internals.DataChannelSubscription =
      service->Subscribe(vtkPVViewProvider::GetDataChannelName(), "ds");
    internals.DataChannelSubscription->GetObservable().subscribe(
      [&internals](const vtkPacket& data) {
        vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "Received rendering data");
        const auto& json = data.GetJSON();
        const auto gid = json.at("gid").get<vtkTypeUInt32>();
        auto view = internals.ObjectStore->FindVTKObject<vtkPVView>(gid);
        if (view)
        {
          auto* dmanager = view->GetDataDeliveryManager();
          dmanager->SetData(json.at("datasets"));
          view->UpdateForRendering();
          view->StillRender();
        }
      });
  }
}

//----------------------------------------------------------------------------
void vtkPVViewProvider::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
