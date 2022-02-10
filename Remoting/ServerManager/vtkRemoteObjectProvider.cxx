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
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkObjectWrapper.h"
#include "vtkPVLogger.h"
#include "vtkProxyDefinitionManager.h"
#include "vtkReactiveCommand.h"
#include "vtkReflection.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"

#include <sstream>

class vtkRemoteObjectProvider::vtkInternals
{
public:
  vtkRemoteObjectProvider* Self;
  vtkSmartPointer<vtkObjectStore> ObjectStore;
  vtkSmartPointer<vtkProxyDefinitionManager> ProxyDefinitionManager;
  rxcpp::composite_subscription Subscription;
  rxcpp::subjects::subject<vtkTypeUInt32> NotifyUpdateSubject;
  rxcpp::subjects::subject<vtkRemoteObjectProvider::vtkProgressItem> ProgressSubject;

  vtkInternals(vtkRemoteObjectProvider* self)
    : Self(self)
  {
  }

  ~vtkInternals()
  {

    // Employ the service's RunLoop to destroy all objects created by this
    // service.  This is done in order to ensure that the (service) thread that
    // created the objects is also the one to delete it.
    auto* service = this->Self->GetService();
    rxcpp::observable<>::just(1)
      .observe_on(service->GetRunLoopScheduler())
      .as_blocking()
      .subscribe([this](int) { this->ObjectStore->UnregisterObjects(); });

    this->Subscription.unsubscribe();
  }

  void Preview(const vtkPacket& packet) {}

  vtkPacket Process(const vtkPacket& packet);

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

  /**
   * Deletes an object.
   */
  void DeleteObject(vtkTypeUInt32 gid);

  /**
   * Invokes a command.
   */
  void InvokeCommand(vtkTypeUInt32 gid, const std::string& command);

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
    this->ObjectStore->FindObject<vtkObjectWrapper>(gid);
  if (objWrapper)
  {
    return objWrapper;
  }

  try
  {
    const auto group = state.at("group").get<std::string>();
    const auto name = state.at("name").get<std::string>();
    auto xml = this->ProxyDefinitionManager->FindProxy(group, name);

    auto node = xml->document_element();
    const std::string className = node.attribute("wrapper_class").as_string("vtkObjectWrapper");
    objWrapper = vtkReflection::CreateInstance<vtkObjectWrapper>(className);
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

    vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "created object (gid=%u, wrapper=%s, object=%s)",
      gid, vtkLogIdentifier(objWrapper), vtkLogIdentifier(objWrapper->GetVTKObject()));
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
  vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "UpdateState(%u, ...)", gid);

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
  vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "UpdateInformation(%u)", gid);

  vtkSmartPointer<vtkObjectWrapper> objWrapper =
    vtkObjectWrapper::SafeDownCast(this->ObjectStore->FindObject(gid));
  if (!objWrapper)
  {
    // TODO: ASYNC send error?
    return vtkJson();
  }

  return objWrapper->UpdateInformation(this->Self);
}

//----------------------------------------------------------------------------
vtkJson vtkRemoteObjectProvider::vtkInternals::UpdatePipeline(vtkTypeUInt32 gid, double time)
{
  vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "UpdatePipeline(%u, %f)", gid, time);

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
void vtkRemoteObjectProvider::vtkInternals::DeleteObject(vtkTypeUInt32 gid)
{
  vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "DeleteObject(%u)", gid);

  this->ObjectStore->UnregisterObject(gid);
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::vtkInternals::InvokeCommand(
  vtkTypeUInt32 gid, const std::string& command)
{
  vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "InvokeCommand(%u, %s)", gid, command.c_str());

  if (auto wrapper = this->ObjectStore->FindObject<vtkObjectWrapper>(gid))
  {
    wrapper->InvokeCommand(command);
  }
}

//----------------------------------------------------------------------------
vtkPacket vtkRemoteObjectProvider::vtkInternals::Process(const vtkPacket& packet)
{
  const auto& json = packet.GetJSON();
  vtkVLogScopeF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "Process: %s", json.dump(-1).c_str());
  const auto type = json.at("type").get<std::string>();

  if (type == "vtk-remote-object-update-state")
  {
    this->UpdateState(json.at("gid").get<vtkTypeUInt32>(), json.at("state"));
    return {};
  }

  if (type == "vtk-remote-object-update-information")
  {
    return this->UpdateInformation(json.at("gid").get<vtkTypeUInt32>());
  }

  if (type == "vtk-remote-object-update-pipeline")
  {
    return this->UpdatePipeline(json.at("gid").get<vtkTypeUInt32>(), json.at("time").get<double>());
  }

  if (type == "vtk-remote-object-delete")
  {
    this->DeleteObject(json.at("gid").get<vtkTypeUInt32>());
    return {};
  }

  if (type == "vtk-remote-object-invoke-command")
  {
    this->InvokeCommand(json.at("gid").get<vtkTypeUInt32>(), json.at("command").get<std::string>());
    return {};
  }

  // TODO: return error?
  vtkLogF(ERROR, "unknown type '%s'", type.c_str());
  return {};
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

    // progress event
    rxvtk::from_event(vtkobject, vtkCommand::ProgressEvent)
      .map([gid, algorithm](const auto& tuple) {
        vtkProgressItem item;
        item.Message = algorithm->GetProgressText() ? algorithm->GetProgressText() : "";
        item.Progress = static_cast<int8_t>(algorithm->GetProgress() * 10) * 10;
        item.GlobalID = gid;
        return item;
      })
      .distinct_until_changed()
      .subscribe(this->ProgressSubject.get_subscriber());
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
  vtkVLogF(
    PARAVIEW_LOG_PROVIDER_VERBOSITY(), "vtkRemoteObjectProvider::~vtkRemoteObjectProvider()");
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
vtkPacket vtkRemoteObjectProvider::DeleteObject(vtkTypeUInt32 gid)
{
  vtkJson message;
  message["type"] = "vtk-remote-object-delete";
  message["gid"] = gid;
  return { message };
}

//----------------------------------------------------------------------------
vtkPacket vtkRemoteObjectProvider::InvokeCommand(vtkTypeUInt32 gid, const std::string& command)
{
  vtkJson message;
  message["type"] = "vtk-remote-object-invoke-command";
  message["gid"] = gid;
  message["command"] = command;
  return { message };
}

//----------------------------------------------------------------------------
std::vector<vtkTypeUInt32> vtkRemoteObjectProvider::ParseNotifyUpdate(const vtkPacket& packet)
{
  return packet.GetJSON().at("gids").get<std::vector<vtkTypeUInt32>>();
}

//----------------------------------------------------------------------------
vtkRemoteObjectProvider::vtkProgressItem vtkRemoteObjectProvider::ParseProgress(
  const vtkPacket& packet)
{
  const auto& json = packet.GetJSON();
  return json.at("progress_item").get<vtkProgressItem>();
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::InitializeInternal(vtkService* service)
{
  assert(service != nullptr);
  auto& internals = (*this->Internals);

  auto observable = service->GetRequestObservable(/*skipEventLoop*/ true);

  // since we want to handle each message in order it is received we create a
  // single processing stream.
  internals.Subscription =
    observable
      .filter([](const vtkServiceReceiver& receiver) {
        auto& json = receiver.GetPacket().GetJSON();
        auto iter = json.find("type");
        return iter != json.end() &&
          iter.value().get<std::string>().find("vtk-remote-object-") != std::string::npos;
      })
      .map([&internals](const vtkServiceReceiver& receiver) {
        internals.Preview(receiver.GetPacket());
        return receiver;
      })
      .observe_on(service->GetRunLoopScheduler())
      .subscribe([&internals](const vtkServiceReceiver& receiver) {
        receiver.Respond(internals.Process(receiver.GetPacket()));
      });

  // publish pipeline-updated notifications periodically.
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

  // can't really buffer since we don't want to skip start/end.
  // just sending all for now, need to figure out better time management.
  internals.ProgressSubject.get_observable().subscribe([service](const auto& item) {
    vtkJson json;
    json["progress_item"] = item;
    service->Publish(vtkRemoteObjectProvider::CHANNEL_PROGRESS(), std::move(json));
  });
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
