/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumService.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThalliumService.h"

#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPacket.h"
#include "vtkThalliumEventualImplementation.h"
#include "vtkThalliumRunLoop.h"
#include "vtkThalliumServiceEndpoint.h"
#include "vtkThalliumServicesEngine.h"
#include "vtkThalliumServicesEngineInternals.h"

#include <map>
#include <mutex>
#include <thallium.hpp>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/map.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

#include "vtk_rxcpp.h" // for rxcpp

// clang-format off
#include VTK_REMOTING_RXCPP(operators/rx-map.hpp)
#include VTK_REMOTING_RXCPP(operators/rx-tap.hpp)
// clang-format on

// TODO switch to a collection of flags/properties
using ThalliumContextType = bool;

class vtkThalliumService::vtkProvider : public thallium::provider<vtkThalliumService::vtkProvider>
{
  using Superclass = thallium::provider<vtkThalliumService::vtkProvider>;

public:
  vtkProvider(vtkThalliumService* service, thallium::engine& engine, uint16_t id)
    : Superclass(engine, id)
    , Service(service)
  {
    auto proc = this->define("send_message", &vtkThalliumService::vtkProvider::SendMessage);
    proc.disable_response();
    this->RemoteProcedures.emplace_back(std::move(proc));

    this->RemoteProcedures.emplace_back(
      this->define("send_request", &vtkThalliumService::vtkProvider::SendRequest));

    this->RemoteProcedures.emplace_back(
      this->define("subscribe", &vtkThalliumService::vtkProvider::Subscribe));

    proc = this->define("unsubscribe", &vtkThalliumService::vtkProvider::Unsubscribe);
    proc.disable_response();
    this->RemoteProcedures.emplace_back(std::move(proc));
  }

  ~vtkProvider() override
  {
    for (auto& proc : this->RemoteProcedures)
    {
      proc.deregister();
    }
  }

  vtkProvider(const vtkProvider&) = delete;
  vtkProvider& operator=(const vtkProvider&) = delete;

protected:
  /**
   * This method is called by vtkThalliumServiceEndpoint when the user sends a
   * packet using `vtkServiceEndpoint::SendMessage`
   */
  void SendMessage(const thallium::request& /*req*/, const vtkPacket& packet)
  {
    this->Service->Dispatch(
      vtkServiceReceiver(this->Service, packet, vtkEventual<vtkJson>::Empty()));
  }

  /**
   * This method is called by vtkThalliumServiceEndpoint when the user sends a
   * packet using `vtkServiceEndpoint::SendRequest`
   */
  void SendRequest(const thallium::request& req, const vtkPacket& packet)
  {
    vtkEventual<vtkJson> eventual(std::make_shared<vtkThalliumEventualImplementation>());
    this->Service->Dispatch(vtkServiceReceiver(this->Service, packet, eventual));

    // wait for the response; this will yield to other RPC requests, is that
    // acceptable? I wonder if we need a mutex for all RPC requests for the same
    // service.
    const vtkPacket reply{ eventual.Wait() };

    // TODO query whether service endpoint is actually remote or not
    // req.get_endpoint() ...
    bool isEndpointRemote = false;
    vtkLogF(INFO, "about-to-send-response");
    req.with_serialization_context(isEndpointRemote).respond(reply);
    vtkLogF(INFO, "sent-response");
  }

  /**
   * This method is called by vtkThalliumServiceEndpoint when the user makes a
   * subscription request using `vtkServiceEndpoint::Subscribe`.
   */
  void Subscribe(const thallium::request& req, const std::string& channel, const std::string& url,
    uint16_t providerId)
  {
    auto engine = this->get_engine();
    auto publish = engine.define("publish").disable_response();
    thallium::provider_handle handle(engine.lookup(url), providerId);
    const bool isRemote = false; // FIXME

    auto subscription = this->Service->GetChannelObservable(channel).subscribe(
      rxcpp::util::apply_to([=](const vtkPacket& packet, bool cached) {
        vtkLogF(TRACE, "publishing %s (cached=%d)", channel.c_str(), cached);
        auto callable = publish.on(handle).with_serialization_context(isRemote);
        callable(channel, packet, cached);
      }));

    const std::lock_guard<thallium::mutex> lk(this->SubscriptionsMutex);
    const auto key = fmt::format("{}:{}:{}", channel, url, providerId);
    if (!this->Subscriptions.emplace(key, subscription).second)
    {
      subscription.unsubscribe();
      req.respond(false);
    }
    else
    {
      req.respond(true);
    }
  }

  void Unsubscribe(const thallium::request& /*req*/, const std::string& channel,
    const std::string& url, uint16_t providerId)
  {
    vtkLogF(TRACE, "unsubcribing %s", channel.c_str());
    const std::lock_guard<thallium::mutex> lk(this->SubscriptionsMutex);

    const auto key = fmt::format("{}:{}:{}", channel, url, providerId);
    auto iter = this->Subscriptions.find(key);
    if (iter != this->Subscriptions.end())
    {
      iter->second.unsubscribe();
      this->Subscriptions.erase(iter);
    }
  }

private:
  std::vector<thallium::remote_procedure> RemoteProcedures;
  vtkThalliumService* Service;

  thallium::mutex SubscriptionsMutex;
  std::map<std::string, rxcpp::composite_subscription> Subscriptions;
  std::map<uint16_t, bool> IsSubscriberRemote;
};

class vtkThalliumService::vtkInternals
{
  static std::string GetThreadName(vtkService* service)
  {
    auto* engine = service->GetEngine();
    auto* controller = engine->GetController();
    const auto rank = controller->GetLocalProcessId();
    const auto numRanks = controller->GetNumberOfProcesses();
    return numRanks > 1 ? fmt::format("{}.{}", service->GetName(), rank) : service->GetName();
  }

public:
  /// This is the runloop for this service.
  std::unique_ptr<vtkThalliumRunLoop> XStreamRunLoop;

  /// A thallium::provider that adds support for the Service-RPC.
  std::unique_ptr<vtkThalliumService::vtkProvider> Provider;

  vtkInternals(vtkThalliumService* self)
    : XStreamRunLoop(new vtkThalliumRunLoop(vtkInternals::GetThreadName(self)))
  {
  }
};

vtkStandardNewMacro(vtkThalliumService);
//----------------------------------------------------------------------------
vtkThalliumService::vtkThalliumService() = default;

//----------------------------------------------------------------------------
vtkThalliumService::~vtkThalliumService() = default;

//----------------------------------------------------------------------------
void vtkThalliumService::InitializeInternal()
{
  // This needs to be done here instead of the constructor,
  // since the service name and engine are not setup until
  // `vtkService::Initialize`.
  this->Internals.reset(new vtkThalliumService::vtkInternals(this));
}

//----------------------------------------------------------------------------
bool vtkThalliumService::StartInternal()
{
  assert(this->Internals);
  auto& internals = *this->Internals;

  auto* engine = vtkThalliumServicesEngine::SafeDownCast(this->GetEngine());
  if (!engine)
  {
    vtkLogF(ERROR, "engine not of correct type!");
    return false;
  }

  auto* sessionInternals = engine->GetInternals();
  auto tengine = sessionInternals->GetEngine();
  const auto providerId = sessionInternals->GetProviderId(this->GetName());

  // starting a service simply ends up being setting up the thallium::provider
  internals.Provider.reset(new vtkThalliumService::vtkProvider(this, tengine, providerId));
  return true;
}

//----------------------------------------------------------------------------
void vtkThalliumService::ShutdownInternal()
{
  vtkLogScopeF(INFO, "%s: ShutdownInternal", vtkLogIdentifier(this));
  assert(this->Internals);
  auto& internals = *this->Internals;
  internals.XStreamRunLoop.reset();
  internals.Provider.reset();
}

//----------------------------------------------------------------------------
const rxcpp::schedulers::run_loop& vtkThalliumService::GetRunLoop() const
{
  auto& internals = *this->Internals;
  if (!internals.XStreamRunLoop)
  {
    throw std::runtime_error("`GetRunLoop` cannot be called after a engine has been shutdown.");
  }
  return internals.XStreamRunLoop->GetRunLoop();
}

//----------------------------------------------------------------------------
void vtkThalliumService::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
