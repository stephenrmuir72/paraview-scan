/*=========================================================================

  Program:   ParaView
  Module:    vtkRemoteObjectProviderViews.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRemoteObjectProviderViews.h"

#include "vtkChannelSubscription.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkObjectWrapper.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVLogger.h"
#include "vtkPVRenderView.h"
#include "vtkPVView.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"

#include <cassert>

class vtkRemoteObjectProviderViews::vtkInternals
{
public:
  vtkSmartPointer<vtkChannelSubscription> DataChannelSubscription;
  bool DataProcessing{ false };
  std::map<vtkTypeUInt32, vtkTimeStamp> PushTimeStamps;
};

vtkObjectFactoryNewMacro(vtkRemoteObjectProviderViews);
//----------------------------------------------------------------------------
vtkRemoteObjectProviderViews::vtkRemoteObjectProviderViews()
  : Internals(new vtkRemoteObjectProviderViews::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkRemoteObjectProviderViews::~vtkRemoteObjectProviderViews() = default;

//----------------------------------------------------------------------------
void vtkRemoteObjectProviderViews::HandleObjectStoreEvent(
  vtkObject* caller, unsigned long eventId, void* callData)
{
  // this is called on service's main thread.
  auto& internals = (*this->Internals);
  if (eventId == vtkObjectStore::RegisterObjectEvent)
  {
    auto* wrapper = vtkObjectWrapper::SafeDownCast(reinterpret_cast<vtkObject*>(callData));
    if (auto* view = wrapper ? vtkPVView::SafeDownCast(wrapper->GetVTKObject()) : nullptr)
    {
      if (internals.DataProcessing)
      {
        vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "Initialize for data processing (%s)",
          vtkLogIdentifier(view));
        view->AddObserver(
          vtkCommand::UpdateEvent, this, &vtkRemoteObjectProviderViews::PushViewData);
      }
      else
      {
        vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "Initialize for rendering (%s)",
          vtkLogIdentifier(view));
        view->AddObserver(
          vtkCommand::RenderEvent, this, &vtkRemoteObjectProviderViews::PushViewImage);
      }
      internals.PushTimeStamps[view->GetGlobalID()].Modified();
    }
  }
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProviderViews::PushViewData(
  vtkObject* caller, unsigned long eventId, void* callData)
{
  auto& internals = (*this->Internals);
  assert("Only supported on data-processing services" && internals.DataProcessing);

  auto* view = vtkPVView::SafeDownCast(caller);
  auto& tstamp = internals.PushTimeStamps.at(view->GetGlobalID());

  auto* dmanager = view->GetDataDeliveryManager();
  auto dataMap = dmanager->GetData(tstamp);
  tstamp.Modified();

  if (!dataMap.empty())
  {
    vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "Pushing rendering data");
    vtkNJson json;
    json["gid"] = view->GetGlobalID();
    this->GetService()->Publish(
      vtkRemoteObjectProviderViews::GetDataChannelName(), vtkPacket{ std::move(json), dataMap });
  }
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProviderViews::PushViewImage(
  vtkObject* caller, unsigned long eventId, void* callData)
{
  auto& internals = (*this->Internals);
  assert("Only supported on rendering services" && !internals.DataProcessing);
  auto* view = vtkPVView::SafeDownCast(caller);
  vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "Pushing rendered image");
  this->GetService()->Publish(
    vtkRemoteObjectProviderViews::GetImageChannelName(), view->GetRenderedResult());
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProviderViews::InitializeInternal(vtkService* service)
{
  this->Superclass::InitializeInternal(service);

  auto store = this->GetObjectStore();
  store->AddObserver(vtkObjectStore::RegisterObjectEvent, this,
    &vtkRemoteObjectProviderViews::HandleObjectStoreEvent);
  store->AddObserver(vtkObjectStore::UnregisterObjectEvent, this,
    &vtkRemoteObjectProviderViews::HandleObjectStoreEvent);

  auto& internals = (*this->Internals);
  internals.DataProcessing = (service->GetName() == "ds"); // TODO: need a better way
  if (!internals.DataProcessing)
  {
    internals.DataChannelSubscription =
      service->Subscribe(vtkRemoteObjectProviderViews::GetDataChannelName(), "ds");
    internals.DataChannelSubscription->GetObservable().subscribe(
      [&internals, this](const vtkPacket& data) {
        vtkVLogScopeF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "Received rendering data");
        const auto& json = data.GetJSON();
        const auto gid = json.at("gid").get<vtkTypeUInt32>();
        auto view = this->GetObjectStore()->FindVTKObject<vtkPVView>(gid);
        if (view)
        {
          auto* dmanager = view->GetDataDeliveryManager();
          dmanager->SetData(data.GetPayload());
        }
      });
  }
}

//----------------------------------------------------------------------------
vtkNJson vtkRemoteObjectProviderViews::PiggybackInformation(vtkObject* object) const
{
  if (auto* rvview = vtkPVRenderView::SafeDownCast(object))
  {
    auto& internals = (*this->Internals);
    if (!internals.DataProcessing)
    {
      auto bbox = rvview->GetVisiblePropBounds();
      std::vector<double> bounds(6, 0.0);
      bbox.GetBounds(&bounds[0]);
      vtkNJson json;
      json["VisiblePropBounds"] = bounds;
      return json;
    }
  }

  return this->Superclass::PiggybackInformation(object);
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProviderViews::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
