/*=========================================================================

  Program:   ParaView
  Module:    vtkRemoteObjectProvider.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRemoteObjectProvider.h"

#include "vtkAlgorithm.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkObjectWrapper.h"
#include "vtkPVXMLElement.h"
#include "vtkProxyDefinitionManager.h"
#include "vtkReactiveCommand.h"
#include "vtkSMInstantiator.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"

class vtkRemoteObjectProvider::vtkInternals
{
public:
  vtkRemoteObjectProvider* Self;
  vtkSmartPointer<vtkObjectStore> ObjectStore;
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkSmartPointer<vtkProxyDefinitionManager> ProxyDefinitionManager;
  std::vector<rxcpp::composite_subscription> Subscriptions;
  rxcpp::subjects::subject<vtkTypeUInt32> NotifyUpdateSubject;

  vtkInternals(vtkRemoteObjectProvider* self)
    : Self(self)
  {
  }

  ~vtkInternals()
  {
    for (auto& subscription : this->Subscriptions)
    {
      subscription.unsubscribe();
    }
  }

  /**
   * Handle state update. This is only called on the service's main thread.
   */
  void UpdateState(vtkTypeUInt32 gid, const vtkJson& state);

  /**
   * Handle update-information requests.
   */
  vtkJson UpdateInformation(vtkTypeUInt32 gid);

  /**
   * Handle update-pipeline requests.
   */
  vtkJson UpdatePipeline(vtkTypeUInt32 gid, double time);

private:
  /**
   * Find (or create) object.
   */
  vtkSmartPointer<vtkObjectWrapper> GetObject(vtkTypeUInt32 gid, const vtkJson& state);

  void SetupObservers(vtkObjectWrapper* wrapper) const;
};

//----------------------------------------------------------------------------
vtkSmartPointer<vtkObjectWrapper> vtkRemoteObjectProvider::vtkInternals::GetObject(
  vtkTypeUInt32 gid, const vtkJson& state)
{
  vtkSmartPointer<vtkObjectWrapper> objWrapper =
    vtkObjectWrapper::SafeDownCast(this->ObjectStore->FindObject(gid));
  if (objWrapper)
  {
    return objWrapper;
  }

  try
  {
    const auto group = state.at("group").get<std::string>();
    const auto name = state.at("name").get<std::string>();
    auto xml = this->ProxyDefinitionManager->FindProxy(group, name);
    if (!xml)
    {
      vtkLogF(ERROR, "Unknown proxy type (%s, %s)", group.c_str(), name.c_str());
      return nullptr;
    }

    // TODO: eventually, vtkProxyDefinitionManager will directly provide us with
    // pugi::xml_node rather than vtkPVXMLElement.
    std::ostringstream stream;
    xml->PrintXML(stream, vtkIndent());
    pugi::xml_document doc;
    auto result = doc.load_string(stream.str().c_str());
    if (!result)
    {
      vtkLogF(ERROR, "unexpected parsing error!");
      abort();
    }

    auto node = doc.document_element();
    const std::string className = node.attribute("wrapper_class").as_string("vtkObjectWrapper");
    objWrapper = vtkObjectWrapper::SafeDownCast(vtkSMInstantiator::CreateInstance(className));
    if (!objWrapper)
    {
      vtkLogF(ERROR, "Unknown wrapper class '%s'", className.c_str());
      return nullptr;
    }

    // Parse XML to ensure the wrapper knows how to update the wrapped-object.
    objWrapper->ReadXMLAttributes(node);

    // Initialize the wrapper with the state.
    objWrapper->Initialize(gid, state, this->Self);

    this->SetupObservers(objWrapper);

    // Register the wrapper so it is associated with the global id.
    this->ObjectStore->RegisterObject(gid, objWrapper);

    vtkLogF(INFO, "created instance of '%s' (gid=%u)", className.c_str(), gid);
    return objWrapper;
  }
  catch (vtkJson::out_of_range&)
  {
    vtkLogF(ERROR, "Missing required keys in state to create object: %s", state.dump().c_str());
    return nullptr;
  }
  catch (vtkJson::type_error&)
  {
    vtkLogF(ERROR, "Invalid state JSON received: %s", state.dump().c_str());
    return nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::vtkInternals::UpdateState(vtkTypeUInt32 gid, const vtkJson& state)
{
  auto object = this->GetObject(gid, state);
  if (!object)
  {
    vtkLogF(ERROR, "Cannot update state: %s", state.dump().c_str());
    return;
  }
  object->UpdateState(state, this->Self);
}

//----------------------------------------------------------------------------
vtkJson vtkRemoteObjectProvider::vtkInternals::UpdateInformation(vtkTypeUInt32 gid)
{
  vtkSmartPointer<vtkObjectWrapper> objWrapper =
    vtkObjectWrapper::SafeDownCast(this->ObjectStore->FindObject(gid));
  if (!objWrapper)
  {
    // TODO: ASYNC send error?
    return vtkJson();
  }

  return objWrapper->UpdateInformation();
}

//----------------------------------------------------------------------------
vtkJson vtkRemoteObjectProvider::vtkInternals::UpdatePipeline(vtkTypeUInt32 gid, double time)
{
  vtkSmartPointer<vtkObjectWrapper> objWrapper =
    vtkObjectWrapper::SafeDownCast(this->ObjectStore->FindObject(gid));
  if (!objWrapper)
  {
    // TODO: ASYNC send error?
    return vtkJson(false);
  }

  return objWrapper->UpdatePipeline(time);
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::vtkInternals::SetupObservers(vtkObjectWrapper* objWrapper) const
{
  auto gid = objWrapper->GetGlobalID();
  auto* vtkobject = objWrapper->GetVTKObject();
  if (auto* algorithm = vtkAlgorithm::SafeDownCast(vtkobject))
  {
    auto* service = this->Self->GetService();
    // FIXME: see async/paraview#2
    // Need to use vtkCommand::RealEndEvent
    rxvtk::from_event(algorithm, vtkCommand::EndEvent)
      .map([gid](const auto& tuple) { return gid; })
      .debounce(std::chrono::milliseconds(500), service->GetRunLoopScheduler())
      .subscribe(this->NotifyUpdateSubject.get_subscriber());
  }
}

//============================================================================
vtkStandardNewMacro(vtkRemoteObjectProvider);
//----------------------------------------------------------------------------
vtkRemoteObjectProvider::vtkRemoteObjectProvider()
  : Internals(new vtkRemoteObjectProvider::vtkInternals(this))
{
}

//----------------------------------------------------------------------------
vtkRemoteObjectProvider::~vtkRemoteObjectProvider()
{
  vtkLogF(INFO, "vtkRemoteObjectProvider::~vtkRemoteObjectProvider()");
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::SetObjectStore(vtkObjectStore* store)
{
  auto& internals = (*this->Internals);
  internals.ObjectStore = store;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkObjectStore> vtkRemoteObjectProvider::GetObjectStore() const
{
  const auto& internals = (*this->Internals);
  return internals.ObjectStore;
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::SetProxyDefinitionManager(vtkProxyDefinitionManager* mgr)
{
  auto& internals = (*this->Internals);
  internals.ProxyDefinitionManager = mgr;
}

//----------------------------------------------------------------------------
vtkProxyDefinitionManager* vtkRemoteObjectProvider::GetProxyDefinitionManager() const
{
  const auto& internals = (*this->Internals);
  return internals.ProxyDefinitionManager;
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::SetController(vtkMultiProcessController* controller)
{
  auto& internals = (*this->Internals);
  internals.Controller = controller;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkRemoteObjectProvider::GetController() const
{
  const auto& internals = (*this->Internals);
  return internals.Controller;
}

//----------------------------------------------------------------------------
vtkPacket vtkRemoteObjectProvider::UpdateState(vtkTypeUInt32 gid, const vtkJson& json)
{
  vtkJson message;
  message["type"] = "vtk-remote-object-update-state";
  message["gid"] = gid;
  message["state"] = json;
  return { message };
}

//----------------------------------------------------------------------------
vtkPacket vtkRemoteObjectProvider::UpdateInformation(vtkTypeUInt32 gid)
{
  vtkJson message;
  message["type"] = "vtk-remote-object-update-information";
  message["gid"] = gid;
  return { message };
}

//----------------------------------------------------------------------------
vtkPacket vtkRemoteObjectProvider::UpdatePipeline(vtkTypeUInt32 gid, double time)
{
  vtkJson message;
  message["type"] = "vtk-remote-object-update-pipeline";
  message["gid"] = gid;
  message["time"] = time;
  return { message };
}

//----------------------------------------------------------------------------
std::vector<vtkTypeUInt32> vtkRemoteObjectProvider::ParseNotifyUpdate(const vtkPacket& packet)
{
  return packet.GetJSON().at("gids").get<std::vector<vtkTypeUInt32>>();
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::InitializeInternal(vtkService* service)
{
  assert(service != nullptr);
  auto& internals = (*this->Internals);
  auto updateStateSubscription =
    service->GetRequestObservable(/*skipEventLoop*/ true)
      .filter([](const vtkServiceReceiver& receiver) {
        const auto& json = receiver.GetPacket().GetJSON();
        auto iter = json.find("type");
        return (iter != json.end() &&
          iter.value().get<std::string>() == "vtk-remote-object-update-state");
      })
      .map([](const vtkServiceReceiver& receiver) {
        const auto& json = receiver.GetPacket().GetJSON();
        vtkLogF(INFO, "received update_state(%u)", json.at("gid").get<vtkTypeUInt32>());
        return receiver.GetPacket();
      })
      .observe_on(service->GetRunLoopScheduler())
      .subscribe([&internals](const vtkPacket& packet) {
        const auto& json = packet.GetJSON();
        vtkLogF(INFO, "%s", json.dump(-1).c_str());
        internals.UpdateState(json.at("gid").get<vtkTypeUInt32>(), json.at("state"));
      });

  auto updateInformationSubscription =
    service->GetRequestObservable(/*skipEventLoop*/ true)
      .filter([](const vtkServiceReceiver& receiver) {
        const auto& json = receiver.GetPacket().GetJSON();
        auto iter = json.find("type");
        return (iter != json.end() &&
          iter.value().get<std::string>() == "vtk-remote-object-update-information");
      })
      .observe_on(service->GetRunLoopScheduler())
      .subscribe([&internals](const vtkServiceReceiver& receiver) {
        const auto& json = receiver.GetPacket().GetJSON();
        auto gid = json.at("gid").get<vtkTypeUInt32>();
        vtkLogF(INFO, "received update_information(%u)", gid);
        receiver.Respond(internals.UpdateInformation(gid));
      });

  auto updatePipelineSubscription =
    service->GetRequestObservable(/*skipEventLoop*/ true)
      .filter([](const vtkServiceReceiver& receiver) {
        const auto& json = receiver.GetPacket().GetJSON();
        auto iter = json.find("type");
        return (iter != json.end() &&
          iter.value().get<std::string>() == "vtk-remote-object-update-pipeline");
      })
      .observe_on(service->GetRunLoopScheduler())
      .subscribe([&internals](const vtkServiceReceiver& receiver) {
        const auto& json = receiver.GetPacket().GetJSON();
        auto gid = json.at("gid").get<vtkTypeUInt32>();
        auto time = json.at("time").get<double>();
        vtkLogScopeF(INFO, "received update_pipeline(%u, time=%f)", gid, time);
        receiver.Respond(internals.UpdatePipeline(gid, time));
      });

  internals.Subscriptions.emplace_back(std::move(updateStateSubscription));
  internals.Subscriptions.emplace_back(std::move(updateInformationSubscription));
  internals.Subscriptions.emplace_back(std::move(updatePipelineSubscription));

  internals.NotifyUpdateSubject.get_observable()
    .buffer_with_time(std::chrono::milliseconds(500), service->GetRunLoopScheduler())
    .filter([](auto& vector) { return !vector.empty(); })
    .subscribe([service](std::vector<vtkTypeUInt32> gids) {
      // gids is a vector of gids that have been updated.
      std::sort(gids.begin(), gids.end());
      gids.erase(std::unique(gids.begin(), gids.end()), gids.end());
      service->Publish(
        vtkRemoteObjectProvider::CHANNEL_NOTIFY_UPDATE(), vtkJson({ { "gids", gids } }));
    });
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
