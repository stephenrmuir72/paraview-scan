/*=========================================================================

  Program:   ParaView
  Module:    vtkService.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkService.h"

#include "vtkChannelSubscription.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNJson.h"
#include "vtkObjectFactory.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkServiceEndpoint.h"
#include "vtkServicesEngine.h"

#include <thread>
namespace
{
constexpr int SERVICE_DISPATCH_RMI_TAG = 229933;

vtkPacket GetEmptyPacket()
{
  return vtkPacket(vtkNJson{ { "__is_initial_value__", true } });
}

struct vtkChannelInfo
{
  bool CachingEnabled{ false };
  rxcpp::subjects::subject<vtkPacket> Subject;
  rxcpp::subjects::behavior<vtkPacket> Behavior{ GetEmptyPacket() };
  rxcpp::observable<std::tuple<vtkPacket, bool>> GetObservable() const
  {
    auto o1 = this->Subject.get_observable().map(
      [](const vtkPacket& packet) { return std::make_tuple(packet, false); });

    auto o2 = this->Behavior.get_observable()
                .filter([](const vtkPacket& packet) {
                  const auto& json = packet.GetJSON();
                  return json.find("__is_initial_value__") == json.end();
                })
                .map([](const vtkPacket& packet) { return std::make_tuple(packet, true); });
    return o1.merge(o2);
  };
};
}

class vtkService::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkServicesEngine* Engine{ nullptr };
  std::string Name;
  vtkSmartPointer<vtkMultiProcessController> Controller;

  // This is the subject that is used to dispatch all messages received by this
  // service.
  rxcpp::subjects::subject<vtkServiceReceiver> RequestSubject;

  ///@{
  /// when an endpoint subscribes to a channel, we track them here. Each
  /// channel-subscriber is set up to send messages to the connected endpoint.
  std::mutex ChannelMutex;
  std::map<std::string, vtkChannelInfo> ChannelSubscribers;
  ///@}

  std::atomic<bool> Initialized{ false };
  std::atomic<bool> Started{ false };
  std::atomic<bool> Shutdown{ false };

  std::vector<unsigned long> RMIs;
};

vtkAbstractObjectFactoryNewMacro(vtkService);
//----------------------------------------------------------------------------
vtkService::vtkService()
  : Internals(new vtkService::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkService::~vtkService()
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
}

//----------------------------------------------------------------------------
void vtkService::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkService::Initialize(vtkServicesEngine* engine, const std::string& name)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  assert(!internals.Initialized);
  internals.Initialized = true;
  internals.Engine = engine;
  internals.Name = name;
  internals.Controller = engine->NewController();

  auto* controller = engine->GetController();
  if (controller->GetLocalProcessId() > 0)
  {
    internals.RMIs.push_back(
      controller->AddRMICallback(&vtkService::RMIDispatch, this, SERVICE_DISPATCH_RMI_TAG));
  }
  this->InitializeInternal();
}

//----------------------------------------------------------------------------
const std::string& vtkService::GetName() const
{
  const auto& internals = (*this->Internals);
  return internals.Name;
}

//----------------------------------------------------------------------------
vtkServicesEngine* vtkService::GetEngine() const
{
  auto& internals = (*this->Internals);
  return internals.Engine;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkService::GetController() const
{
  const auto& internals = (*this->Internals);
  return internals.Controller;
}

//----------------------------------------------------------------------------
void vtkService::RMIDispatch(
  void* localArg, void* remoteArg, int remoteArgLength, int remoteProcessId)
{
  // TODO: avoid repeated deserialization for each service.
  vtkService* self = reinterpret_cast<vtkService*>(localArg);
  const uint8_t* buffer = reinterpret_cast<uint8_t*>(remoteArg);
  nlohmann::json json = nlohmann::json::from_cbor(buffer, buffer + remoteArgLength);
  if (json.at("__vtk_service_id__") == self->GetName())
  {
    json.erase("__vtk_service_id__");

    // TODO: dispatch on RPC pool for consistency across MPI ranks.
    self->Dispatch(
      vtkServiceReceiver(self, vtkPacket(std::move(json)), vtkEventual<vtkNJson>::Empty()));
  }
}

//----------------------------------------------------------------------------
rxcpp::observable<vtkServiceReceiver> vtkService::GetRequestObservable(bool skipEventLoop) const
{
  const auto& internals = *this->Internals;
  if (skipEventLoop)
  {
    return internals.RequestSubject.get_observable();
  }

  return internals.RequestSubject.get_observable().observe_on(this->GetRunLoopScheduler());
}

//----------------------------------------------------------------------------
void vtkService::Dispatch(vtkServiceReceiver&& message)
{
  const auto& internals = *this->Internals;
  auto* controller = internals.Engine->GetController();
  if (controller->GetLocalProcessId() == 0 && controller->GetNumberOfProcesses() > 1)
  {
    auto& packet = message.GetPacket();
    vtkNJson json = packet.GetJSON();
    json["__vtk_service_id__"] = this->GetName();
    std::vector<uint8_t> buffer = nlohmann::json::to_cbor(json);
    controller->TriggerRMIOnAllChildren(
      &buffer[0], static_cast<int>(buffer.size()), SERVICE_DISPATCH_RMI_TAG);
  }
  internals.RequestSubject.get_subscriber().on_next(std::move(message));
}

//----------------------------------------------------------------------------
void vtkService::EnableChannelCache(const std::string& channelName) const
{
  auto& internals = *this->Internals;
  const std::lock_guard<std::mutex> guard(internals.ChannelMutex);

  auto iter = internals.ChannelSubscribers.emplace(channelName, vtkChannelInfo()).first;
  iter->second.CachingEnabled = true;
}

//----------------------------------------------------------------------------
void vtkService::ClearChannelCache(const std::string& channelName) const
{
  auto& internals = *this->Internals;
  const std::lock_guard<std::mutex> guard(internals.ChannelMutex);
  if (internals.Shutdown)
  {
    // all channels are already deleted.
    return;
  }

  auto iter = internals.ChannelSubscribers.find(channelName);
  if (iter != internals.ChannelSubscribers.end())
  {
    iter->second.CachingEnabled = false;
    iter->second.Behavior.get_subscriber().on_next(GetEmptyPacket());
  }
}

//----------------------------------------------------------------------------
rxcpp::observable<std::tuple<vtkPacket, bool>> vtkService::GetChannelObservable(
  const std::string& channelName)
{
  auto& internals = *this->Internals;
  const std::lock_guard<std::mutex> guard(internals.ChannelMutex);
  if (internals.Shutdown)
  {
    vtkLogF(ERROR, "GetChannelObservable(%s) called after shutdown!", channelName.c_str());
    return rxcpp::sources::never<std::tuple<vtkPacket, bool>>();
  }

  auto iter = internals.ChannelSubscribers.emplace(channelName, vtkChannelInfo()).first;
  return iter->second.GetObservable();
}

//----------------------------------------------------------------------------
void vtkService::Publish(const std::string& channelName, const vtkPacket& packet) const
{
  auto& internals = *this->Internals;
  if (internals.Shutdown)
  {
    // can't publish once shutdown.
    return;
  }

  if (!internals.Started)
  {
    vtkLogF(ERROR, "Cannot publish messages before the service has been started!");
    return;
  }

  std::unique_lock<std::mutex> guard(internals.ChannelMutex);
  auto iter = internals.ChannelSubscribers.emplace(channelName, vtkChannelInfo()).first;
  const auto& info = iter->second;
  const auto subscriber =
    info.CachingEnabled ? info.Behavior.get_subscriber() : info.Subject.get_subscriber();
  guard.unlock();
  subscriber.on_next(packet);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkChannelSubscription> vtkService::Subscribe(
  const std::string& channel, const std::string& serviceName) const
{
  auto* engine = this->GetEngine();
  if (auto otherService = engine->GetService(serviceName))
  {
    // if otherService exists, then two services are on the same process. We can
    // easily communicate between those services without going to
    // service-endpoints. This is especially crucial in MPI environments since
    // services running on satellites cannot communicate with one another
    // through endpoints.
    auto subscription = vtk::TakeSmartPointer(vtkChannelSubscription::New());
    subscription->Initialize(nullptr, channel,
      otherService->GetChannelObservable(channel)
        .map(rxcpp::util::apply_to([](vtkPacket packet, bool /*cached*/) { return packet; }))
        .observe_on(this->GetRunLoopScheduler()),
      vtkEventual<bool>{ true });
    return subscription;
  }

  auto* controller = engine->GetController();
  if (controller->GetLocalProcessId() == 0)
  {
    auto ep = engine->GetServiceEndpoint(serviceName);
    return ep ? ep->Subscribe(channel, rxcpp::observe_on_run_loop(this->GetRunLoop())) : nullptr;
  }

  // TODO: this is the "reduction" use-case. We have a M-rank server sending data back
  // to a service on a remote process. need to figure out how we're doing to
  // support that eventually.
  vtkLogF(ERROR, "encountered communcation request not supported yet :(");
  abort();
}

//----------------------------------------------------------------------------
bool vtkService::Start()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (internals.Started)
  {
    vtkLogF(ERROR, "service already started!");
    return false;
  }

  if (internals.Shutdown)
  {
    vtkLogF(ERROR, "cannot restart a stopped service!");
    return false;
  }

  if (!this->GetEngine())
  {
    vtkLogF(ERROR, "cannot start without a engine!");
    return false;
  }

  internals.Started = true;
  return this->StartInternal();
}

//----------------------------------------------------------------------------
void vtkService::Shutdown()
{
  vtkLogScopeF(INFO, "%s: Shutdown", vtkLogIdentifier(this));
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (!internals.Started || internals.Shutdown)
  {
    return;
  }
  // Stop accepting messages/requests.
  internals.RequestSubject.get_subscriber().unsubscribe();

  // Stop publishing messages
  std::unique_lock<std::mutex> guard(internals.ChannelMutex);
  for (auto& pair : internals.ChannelSubscribers)
  {
    pair.second.Subject.get_subscriber().on_completed();
    pair.second.Behavior.get_subscriber().on_completed();
  }
  internals.ChannelSubscribers.clear();
  guard.unlock();

  internals.Started = false;
  internals.Shutdown = true;
  this->ShutdownInternal();

  // unregister all rmis.
  auto* controller = internals.Engine->GetController();
  for (const auto& rmiId : internals.RMIs)
  {
    controller->RemoveRMICallback(rmiId);
  }
  internals.RMIs.clear();
  internals.Engine = nullptr;
}

//----------------------------------------------------------------------------
void vtkService::UnsetEngine()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.Engine = nullptr;
}

//----------------------------------------------------------------------------
vtkServiceReceiver::vtkServiceReceiver(
  vtkService* service, const vtkPacket& packet, const vtkEventual<vtkNJson>& response)
  : Service(service)
  , Response(response)
  , Packet(packet)
{
}

//----------------------------------------------------------------------------
void vtkServiceReceiver::Respond(const vtkNJson& json) const
{
  this->Response.SetValue(json);
}

//----------------------------------------------------------------------------
void vtkServiceReceiver::Respond(const vtkPacket& packet) const
{
  this->Respond(packet.GetJSON());
}

//----------------------------------------------------------------------------
void vtkServiceReceiver::Publish(const std::string& channel, const vtkPacket& packet) const
{
  if (this->Service)
  {
    this->Service->Publish(channel, packet);
  }
}
