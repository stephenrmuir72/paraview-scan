/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumServiceEndpoint.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThalliumServiceEndpoint.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPacket.h"
#include "vtkThalliumEventualImplementation.h"
#include "vtkThalliumServicesEngine.h"
#include "vtkThalliumServicesEngineInternals.h"

#include <cassert>
#include <thallium.hpp>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/map.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <tuple>

/**
 * This is an internal `thallium::provider` used by vtkThalliumServiceEndpoint.
 * To support publish-subscribe, the vtkThalliumService make a reverse connection
 * to the vtkThalliumServiceEndpoint and then pushes messages to this provider.
 */
class vtkThalliumServiceEndpoint::vtkProvider
  : public thallium::provider<vtkThalliumServiceEndpoint::vtkProvider>
{
  using Superclass = thallium::provider<vtkThalliumServiceEndpoint::vtkProvider>;

public:
  vtkProvider(thallium::engine& engine, vtkThalliumServiceEndpoint* endpoint)
    : Superclass(engine, ++vtkThalliumServiceEndpoint::vtkProvider::NextId)
    , Endpoint(endpoint)
  {
    vtkLogF(INFO, "vtkProvider(%p)", this);
    this->RProcedures.push_back(
      this->define("publish", &vtkThalliumServiceEndpoint::vtkProvider::Publish)
        .disable_response());
  }

  void Reset()
  {
    vtkLogF(INFO, "vtkProvider::Reset(%p)", this);
    for (auto& proc : this->RProcedures)
    {
      proc.deregister();
    }
    this->RProcedures.clear();
    this->Endpoint = nullptr;
  }

private:
  /**
   * This method gets called by `vtkService` when a message is published on a
   * named channel.
   */
  void Publish(const thallium::request& /*req*/, const std::string& channel,
    const vtkPacket& packet, bool cached) const
  {
    vtkLogF(TRACE, "got data on channel %s (cached=%d)", channel.c_str(), cached);
    if (this->Endpoint)
    {
      this->Endpoint->DispatchOnChannel(channel, packet, cached);
    }
  }

  vtkThalliumServiceEndpoint* Endpoint;
  std::vector<thallium::remote_procedure> RProcedures;
  static uint16_t NextId;
};

uint16_t vtkThalliumServiceEndpoint::vtkProvider::NextId;

class vtkThalliumServiceEndpoint::vtkInternals
{
public:
  thallium::engine Engine;

  // the handle is available once a connection is established.
  thallium::eventual<thallium::provider_handle> ServiceHandle;

  // this counter is used to identify align requests and their replies.
  std::unique_ptr<vtkThalliumServiceEndpoint::vtkProvider> Provider;
  bool IsServiceRemote{ false };

  vtkInternals(thallium::engine& engine, vtkThalliumServiceEndpoint* /*self*/)
    : Engine(engine)
  {
  }
};

vtkStandardNewMacro(vtkThalliumServiceEndpoint);
//----------------------------------------------------------------------------
vtkThalliumServiceEndpoint::vtkThalliumServiceEndpoint() = default;

//----------------------------------------------------------------------------
vtkThalliumServiceEndpoint::~vtkThalliumServiceEndpoint()
{
  vtkLogF(INFO, "vtkThalliumServiceEndpoint::~vtkThalliumServiceEndpoint(%p) %s", this,
    this->GetServiceName().c_str());
}

//----------------------------------------------------------------------------
void vtkThalliumServiceEndpoint::InitializeInternal()
{
  auto* engine = vtkThalliumServicesEngine::SafeDownCast(this->GetEngine());
  if (!engine)
  {
    vtkLogF(ERROR, "engine not of correct type!");
    std::terminate();
  }

  auto* sessionInternals = engine->GetInternals();
  auto tlEngine = sessionInternals->GetEngine();

  // This needs to be done here instead of the constructor,
  // since the engine is not setup until `vtkServiceEndpoint::Initialize`.
  this->Internals.reset(new vtkThalliumServiceEndpoint::vtkInternals(tlEngine, this));
}

//----------------------------------------------------------------------------
vtkEventual<bool> vtkThalliumServiceEndpoint::ConnectInternal(
  const std::string& serviceName, const std::string& url)
{
  auto& internals = *this->Internals;
  // register thallium::provider on the engine to handle published messages.
  internals.Provider.reset(new vtkThalliumServiceEndpoint::vtkProvider(internals.Engine, this));

  auto impl = std::make_shared<vtkThalliumEventualImplementation>();

  auto ep = internals.Engine.lookup(url);
  auto handshake = internals.Engine.define("handshake");
  internals.Engine.get_handler_pool().make_thread(
    [handshake, ep, url, serviceName, impl, &internals]() {
      std::string token = "connection-secret";
      uint16_t providerId = handshake.on(ep)(token, serviceName); // TODO: add timeout
      if (providerId)
      {
        internals.ServiceHandle.set_value(thallium::provider_handle(ep, providerId));
        impl->SetValue(true);
      }
      else
      {
        internals.ServiceHandle.set_value(thallium::provider_handle());
        impl->SetValue(false);
      }
    },
    thallium::anonymous{});
  // TODO update internals.IsServiceRemote based on connection metadata?
  internals.IsServiceRemote = false;
  return vtkEventual<bool>(impl);
}

//----------------------------------------------------------------------------
void vtkThalliumServiceEndpoint::ShutdownInternal()
{
  vtkLogF(INFO, "vtkThalliumServiceEndpoint::ShutdownInternal");
  auto& internals = *this->Internals;
  internals.Provider->Reset();
  internals.Provider.reset();
  internals.ServiceHandle.reset();
  internals.ServiceHandle.set_value(thallium::provider_handle());
}

//----------------------------------------------------------------------------
void vtkThalliumServiceEndpoint::SendMessageInternal(const vtkPacket& packet) const
{
  auto& internals = *this->Internals;
  auto phandle = internals.ServiceHandle.wait();
  if (phandle.is_null())
  {
    vtkLogF(ERROR, "connection has failed; cannot sent message");
    return;
  }

  auto send_message = internals.Engine.define("send_message").disable_response();
  const bool isServiceRemote = internals.IsServiceRemote;
  vtkLogF(TRACE, "sending message to %s", this->GetServiceName().c_str());
  auto callable = send_message.on(phandle).with_serialization_context(isServiceRemote);
  callable(packet);
}

//----------------------------------------------------------------------------
vtkEventual<vtkPacket> vtkThalliumServiceEndpoint::SendRequestInternal(
  const vtkPacket& packet) const
{
  auto& internals = *this->Internals;
  auto phandle = internals.ServiceHandle.wait();
  if (phandle.is_null())
  {
    vtkLogF(ERROR, "connection has failed; cannot send request");
    return vtkEventual<vtkPacket>::Empty();
  }

  auto send_request = internals.Engine.define("send_request");
  const bool isServiceRemote = internals.IsServiceRemote;

  vtkLogF(TRACE, "sending request to %s", this->GetServiceName().c_str());
  // callable will be called on the (remote) service ...
  auto callable = send_request.on(phandle).with_serialization_context(isServiceRemote);
  auto response = std::make_shared<thallium::async_response>(callable.async(packet));

  auto impl = std::make_shared<vtkThalliumEventualImplementation>();
  // create a task to populate the eventual when the response is received.
  internals.Engine.get_handler_pool().make_thread(
    [=]() {
      auto packet = response->wait().as<vtkPacket>();
      impl->SetValue(packet.GetJSON());
    },
    thallium::anonymous{});
  return vtkEventual<vtkPacket>(std::move(impl));
}

//----------------------------------------------------------------------------
vtkEventual<bool> vtkThalliumServiceEndpoint::SubscribeInternal(const std::string& channel)
{
  auto& internals = *this->Internals;
  auto phandle = internals.ServiceHandle.wait();
  if (phandle.is_null())
  {
    vtkLogF(ERROR, "connection has failed; cannot sent message");
    return vtkEventual<bool>(false);
  }

  // Send a subscription request.
  auto subscribe = internals.Engine.define("subscribe");
  auto providerId = internals.Provider->get_provider_id();
  const std::string url = internals.Engine.self();

  vtkLogF(INFO, "subscribing to %s on %s", channel.c_str(), this->GetServiceName().c_str());
  auto callable = subscribe.on(phandle);
  auto response =
    std::make_shared<thallium::async_response>(callable.async(channel, url, providerId));
  auto impl = std::make_shared<vtkThalliumEventualImplementation>();
  internals.Engine.get_handler_pool().make_thread(
    [=]() { impl->SetValue(response->wait().as<bool>()); }, thallium::anonymous{});
  return vtkEventual<bool>(impl);
}

//----------------------------------------------------------------------------
void vtkThalliumServiceEndpoint::UnsubscribeInternal(const std::string& channel)
{
  auto& internals = *this->Internals;
  if (!internals.Provider)
  {
    return;
  }

  auto phandle = internals.ServiceHandle.wait();
  if (phandle.is_null())
  {
    return;
  }

  // Send a request for unsubscription.
  auto unsubscribe = internals.Engine.define("unsubscribe").disable_response();
  const std::string url = internals.Engine.self();
  auto providerId = internals.Provider->get_provider_id();
  vtkLogF(
    INFO, "unsubscribing from channel %s on %s", channel.c_str(), this->GetServiceName().c_str());
  unsubscribe.on(phandle)(channel, url, providerId);
}

//----------------------------------------------------------------------------
void vtkThalliumServiceEndpoint::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
