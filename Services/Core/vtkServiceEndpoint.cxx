/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkServiceEndpoint.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkServiceEndpoint.h"

#include "vtkChannelSubscription.h"
#include "vtkLogger.h"
#include "vtkNJson.h"
#include "vtkObjectFactory.h"
#include "vtkPacket.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkServicesEngine.h"
#include "vtkSmartPointer.h"

namespace
{
vtkPacket GetEmptyPacket()
{
  return vtkPacket(vtkNJson{ { "__is_initial_value__", true } });
}
}

class vtkServiceEndpoint::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkServicesEngine* Engine{ nullptr };
  std::atomic<bool> Connected{ false };
  std::atomic<bool> Shutdown{ false };
  std::string ServiceName;
  std::string Url;

  rxcpp::subjects::subject<std::tuple<std::string, vtkPacket>> ChannelSubject;

  std::mutex SubscriptionsMutex;
  struct SubscriptionInfo
  {
    int32_t ReferenceCount{ 0 };
    vtkEventual<bool> Status{ false };

    rxcpp::subjects::subject<vtkPacket> Subject;
    rxcpp::subjects::behavior<vtkPacket> Behavior{ ::GetEmptyPacket() };
    rxcpp::observable<vtkPacket> GetObservable() const
    {
      auto o1 = this->Subject.get_observable();
      auto o2 = this->Behavior.get_observable().filter([](const vtkPacket& packet) {
        const auto& data = packet.GetJSON();
        return data.find("__is_initial_value__") == data.end();
      });
      return o1.merge(o2);
    }

    void operator=(const SubscriptionInfo&) = delete;
    SubscriptionInfo(const SubscriptionInfo&) = delete;
    SubscriptionInfo() = default;
    ~SubscriptionInfo() { this->Subject.get_subscriber().on_completed(); }
  };

  std::map<std::string, SubscriptionInfo> Subscriptions;
};

vtkAbstractObjectFactoryNewMacro(vtkServiceEndpoint);
//----------------------------------------------------------------------------
vtkServiceEndpoint::vtkServiceEndpoint()
  : Internals(new vtkServiceEndpoint::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkServiceEndpoint::~vtkServiceEndpoint()
{
  const auto& internals = (*this->Internals);
  vtkLogIfF(ERROR, internals.Connected || !internals.Shutdown, "Did you forget to call shutdown?");
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
}

//----------------------------------------------------------------------------
void vtkServiceEndpoint::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkServiceEndpoint::Initialize(
  vtkServicesEngine* engine, const std::string& serviceName, const std::string& sessionUrl)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.Engine = engine;
  internals.ServiceName = serviceName;
  internals.Url = sessionUrl;
  this->InitializeInternal();
}

//----------------------------------------------------------------------------
void vtkServiceEndpoint::UnsetEngine()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.Engine = nullptr;
}

//----------------------------------------------------------------------------
vtkServicesEngine* vtkServiceEndpoint::GetEngine() const
{
  auto& internals = (*this->Internals);
  return internals.Engine;
}

//----------------------------------------------------------------------------
vtkEventual<bool> vtkServiceEndpoint::Connect()
{
  auto& internals = (*this->Internals);
  if (internals.Connected)
  {
    vtkLogF(TRACE, "already connected to a service!");
    return vtkEventual<bool>(true);
  }
  if (internals.Shutdown)
  {
    vtkLogF(ERROR, "Cannot reused a stale endpoint!");
    return vtkEventual<bool>(false);
  }

  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.Connected = true;
  return this->ConnectInternal(internals.ServiceName, internals.Url);
}

//----------------------------------------------------------------------------
void vtkServiceEndpoint::Shutdown()
{
  vtkLogScopeF(INFO, "%s: Shutdown", vtkLogIdentifier(this));
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (!internals.Connected || internals.Shutdown)
  {
    return;
  }
  internals.Connected = false;
  internals.Shutdown = true;

  {
    // release all subscriptions
    std::lock_guard<std::mutex> lock(internals.SubscriptionsMutex);
    internals.Subscriptions.clear();
  }
  this->ShutdownInternal();
}

//----------------------------------------------------------------------------
const std::string& vtkServiceEndpoint::GetServiceName() const
{
  const auto& internals = (*this->Internals);
  return internals.ServiceName;
}

//----------------------------------------------------------------------------
const std::string& vtkServiceEndpoint::GetServiceUrl() const
{
  const auto& internals = (*this->Internals);
  return internals.Url;
}

//----------------------------------------------------------------------------
void vtkServiceEndpoint::SendMessage(const vtkPacket& packet) const
{
  const auto& internals = (*this->Internals);
  if (internals.Shutdown)
  {
    return;
  }
  if (!internals.Connected)
  {
    vtkLogF(ERROR, "not connected yet!");
    return;
  }

  this->SendMessageInternal(packet);
}

//----------------------------------------------------------------------------
vtkEventual<vtkPacket> vtkServiceEndpoint::SendRequest(const vtkPacket& packet) const
{
  auto& internals = (*this->Internals);
  if (internals.Shutdown)
  {
    return vtkEventual<vtkPacket>::Empty();
  }

  if (!internals.Connected)
  {
    vtkLogF(ERROR, "not connected yet!");
    return vtkEventual<vtkPacket>::Empty();
  }

  return this->SendRequestInternal(packet);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkChannelSubscription> vtkServiceEndpoint::Subscribe(const std::string& channel)
{
  auto& internals = (*this->Internals);
  return this->Subscribe(channel, this->GetEngine()->GetCoordination());
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkChannelSubscription> vtkServiceEndpoint::Subscribe(
  const std::string& channel, const rxcpp::observe_on_one_worker& coordination)
{
  auto& internals = (*this->Internals);
  if (internals.Shutdown)
  {
    return nullptr;
  }

  std::lock_guard<std::mutex> lock(internals.SubscriptionsMutex);
  auto& info = internals.Subscriptions[channel];
  info.ReferenceCount += 1;
  if (info.ReferenceCount == 1)
  {
    info.Status = this->SubscribeInternal(channel);
  }

  auto observable = info.GetObservable().observe_on(coordination);

  /*
      internals.ChannelSubject.get_observable()
        .filter([channel](const std::tuple<std::string, vtkPacket>& tuple) {
          return std::get<0>(tuple) == channel;
        })
        .map([](const std::tuple<std::string, vtkPacket>& tuple) { return std::get<1>(tuple); })
        .observe_on(coodination);
        */
  auto subscription = vtk::TakeSmartPointer(vtkChannelSubscription::New());
  subscription->Initialize(this, channel, std::move(observable), info.Status);
  return subscription;
}

//----------------------------------------------------------------------------
void vtkServiceEndpoint::Unsubscribe(const std::string& channel)
{
  auto& internals = (*this->Internals);
  if (internals.Shutdown)
  {
    return;
  }
  if (!internals.Connected)
  {
    vtkLogF(ERROR, "not connected yet!");
    return;
  }

  std::lock_guard<std::mutex> lock(internals.SubscriptionsMutex);
  auto iter = internals.Subscriptions.find(channel);
  if (iter == internals.Subscriptions.end())
  {
    vtkLogF(ERROR, "no such channel to unsubscribe: %s", channel.c_str());
    return;
  }
  const auto count = --iter->second.ReferenceCount;
  if (count == 0)
  {
    internals.Subscriptions.erase(iter);
    this->UnsubscribeInternal(channel);
  }
}

//----------------------------------------------------------------------------
void vtkServiceEndpoint::DispatchOnChannel(
  const std::string& channel, const vtkPacket& packet, bool cached) const
{
  auto& internals = (*this->Internals);
  std::unique_lock<std::mutex> lock(internals.SubscriptionsMutex);
  auto iter = internals.Subscriptions.find(channel);
  if (iter != internals.Subscriptions.end())
  {
    auto cachedSubscriber = iter->second.Behavior.get_subscriber();
    auto subscriber = iter->second.Subject.get_subscriber();
    lock.unlock();

    if (cached)
    {
      cachedSubscriber.on_next(packet);
    }
    else
    {
      cachedSubscriber.on_next(::GetEmptyPacket());
      subscriber.on_next(packet);
    }
  }
}
