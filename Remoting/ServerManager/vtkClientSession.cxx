/*=========================================================================

  Program:   ParaView
  Module:    vtkClientSession.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientSession.h"

#include "vtkChannelSubscription.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVCoreApplication.h"
#include "vtkRemoteObjectProvider.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkSMProxy.h"
#include "vtkSMReaderFactory.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkServiceEndpoint.h"
#include "vtkServicesEngine.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"

#include <cassert>

class vtkClientSession::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  bool IsRemote{ false };
  std::string URL;
  vtkSmartPointer<vtkServiceEndpoint> DSEndpoint;
  vtkSmartPointer<vtkServiceEndpoint> RSEndpoint;
  vtkSmartPointer<vtkSMSessionProxyManager> ProxyManager;
  vtkSmartPointer<vtkSMReaderFactory> ReaderFactory;
  std::vector<vtkSmartPointer<vtkChannelSubscription>> ProgressSubscriptions;

  rxcpp::subjects::subject<std::tuple<std::string, vtkRemoteObjectProvider::vtkProgressItem>>
    ProgressSubject;

  /**
   * Called when the session is ready.
   */
  void InitializeSession(vtkClientSession* self);

  std::vector<vtkServiceEndpoint*> GetEndpoints(int destination) const
  {
    std::vector<vtkServiceEndpoint*> endpoints;
    if ((destination & vtkClientSession::DATA_SERVER) != 0)
    {
      endpoints.push_back(this->DSEndpoint);
    }
    if ((destination & vtkClientSession::RENDER_SERVER) != 0)
    {
      endpoints.push_back(this->RSEndpoint);
    }
    return endpoints;
  }

  ~vtkInternals()
  {
    if (this->DSEndpoint)
    {
      this->DSEndpoint->Shutdown();
      this->DSEndpoint->GetEngine()->UnRegisterServiceEndpoint(this->DSEndpoint);
    }
    if (this->RSEndpoint)
    {
      this->RSEndpoint->Shutdown();
      this->RSEndpoint->GetEngine()->UnRegisterServiceEndpoint(this->RSEndpoint);
    }
  }
};

//----------------------------------------------------------------------------
void vtkClientSession::vtkInternals::InitializeSession(vtkClientSession* self)
{
  assert(this->DSEndpoint && this->RSEndpoint);
  this->ProxyManager = vtk::TakeSmartPointer(vtkSMSessionProxyManager::New(self));
  this->ReaderFactory = vtk::TakeSmartPointer(vtkSMReaderFactory::New());
  this->ReaderFactory->SetSession(self);
  this->ReaderFactory->RegisterPrototypesFromGroup("sources");
}

//============================================================================
const std::initializer_list<vtkTypeUInt32> vtkClientSession::ServiceTypeList = {
  vtkClientSession::DATA_SERVER, vtkClientSession::RENDER_SERVER, vtkClientSession::CLIENT
};

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkClientSession);
//----------------------------------------------------------------------------
vtkClientSession::vtkClientSession()
  : Internals(new vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkClientSession::~vtkClientSession() = default;

//----------------------------------------------------------------------------
rxcpp::observable<bool> vtkClientSession::Connect(const std::string& url)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  const auto engineURL = url.empty() ? url : vtkPVCoreApplication::GetEngineURL(url);
  auto* pvapp = vtkPVCoreApplication::GetInstance();
  assert(pvapp != nullptr);
  auto* engine = pvapp->GetServicesEngine();
  internals.IsRemote =
    (engineURL.empty() == false) && (engine->GetUrl() != engineURL); // FIXME: confirm
  internals.URL = url;
  if (pvapp->GetRank() == 0)
  {
    auto behavior = std::make_shared<rxcpp::subjects::behavior<vtkVariant>>(vtkVariant());

    internals.DSEndpoint = engine->CreateServiceEndpoint("ds", engineURL);
    auto o1 = internals.DSEndpoint->Connect().GetObservable();

    internals.RSEndpoint = engine->CreateServiceEndpoint("rs", engineURL);
    auto o2 = internals.RSEndpoint->Connect().GetObservable();

    o1.combine_latest(o2)
      .observe_on(engine->GetCoordination())
      .map([](const std::tuple<bool, bool>& status) {
        return std::get<0>(status) && std::get<1>(status);
      })
      .subscribe([behavior, &internals, this](bool status) {
        if (status)
        {
          internals.InitializeSession(this);
          this->InitializeServiceEndpoint(internals.DSEndpoint);
          this->InitializeServiceEndpoint(internals.RSEndpoint);
        }
        behavior->get_subscriber().on_next(vtkVariant(status ? 1 : 0));
      });

    return behavior->get_observable()
      .filter([](const vtkVariant& variant) { return variant.IsValid(); })
      .take(1)
      .map([behavior](const vtkVariant& variant) {
        // capture behavior to ensure it hangs around until the returned value hangs around.
        (void)behavior;
        return variant.ToInt() != 0;
      });
  }

  return rxcpp::sources::never<bool>();
}

//----------------------------------------------------------------------------
const std::string& vtkClientSession::GetURL() const
{
  const auto& internals = (*this->Internals);
  return internals.URL;
}

//----------------------------------------------------------------------------
void vtkClientSession::InitializeServiceEndpoint(vtkServiceEndpoint* endpoint)
{
  const std::string serviceName = endpoint->GetServiceName();
  auto& internals = (*this->Internals);
  auto subscription = endpoint->Subscribe(vtkRemoteObjectProvider::CHANNEL_PROGRESS());
  subscription->GetObservable().subscribe([&internals, serviceName](const vtkPacket& packet) {
    auto item = vtkRemoteObjectProvider::ParseProgress(packet);
    auto* proxy = internals.ProxyManager->FindProxy(item.GlobalID);
    if (item.Message.empty() && proxy != nullptr)
    {
      item.Message = proxy->GetLogNameOrDefault();
    }
    internals.ProgressSubject.get_subscriber().on_next(std::make_tuple(serviceName, item));
  });
  internals.ProgressSubscriptions.push_back(subscription);
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkClientSession::GetProxyManager() const
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (!internals.ProxyManager)
  {
    vtkLogF(ERROR, "No proxymanager present. Is the session connected yet?");
    return nullptr;
  }
  return internals.ProxyManager;
}

//----------------------------------------------------------------------------
vtkSMReaderFactory* vtkClientSession::GetReaderFactory() const
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (!internals.ReaderFactory)
  {
    vtkLogF(ERROR, "No ReaderFactory present. Is the session connected yet?");
    return nullptr;
  }

  return internals.ReaderFactory;
}

//----------------------------------------------------------------------------
void vtkClientSession::SendMessage(vtkTypeUInt32 destination, const vtkPacket& packet) const
{
  const auto& internals = (*this->Internals);
  for (auto& endpoint : internals.GetEndpoints(destination))
  {
    endpoint->SendMessage(packet);
  }
}

//----------------------------------------------------------------------------
vtkEventual<vtkPacket> vtkClientSession::SendRequest(
  vtkTypeUInt32 destination, const vtkPacket& packet) const
{
  const auto& internals = (*this->Internals);
  auto* rootEndpoint =
    internals.GetEndpoints(vtkClientSession::GetRootDestination(destination)).front();
  for (auto* endpoint : internals.GetEndpoints(destination))
  {
    if (endpoint != rootEndpoint)
    {
      endpoint->SendMessage(packet);
    }
  }
  return rootEndpoint->SendRequest(packet);
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkClientSession::GetRootDestination(vtkTypeUInt32 destination)
{
  switch (destination)
  {
    case (DATA_SERVER | RENDER_SERVER):
      return DATA_SERVER;

    case (CLIENT | DATA_SERVER):
      // return CLIENT; // FIXME: ASYNC
      return DATA_SERVER;

    case (CLIENT | RENDER_SERVER):
      // return CLIENT; FIXME:ASYNC
      return RENDER_SERVER;

    case (CLIENT | DATA_SERVER | RENDER_SERVER):
      // return CLIENT; // FIXME: ASYNC
      return DATA_SERVER;

    case DATA_SERVER:
    case RENDER_SERVER:
    case CLIENT:
      return destination;

    default:
      abort();
  }
}

//----------------------------------------------------------------------------
rxcpp::observe_on_one_worker vtkClientSession::GetCoordination() const
{
  auto* pvapp = vtkPVCoreApplication::GetInstance();
  return pvapp->GetServicesEngine()->GetCoordination();
}

//----------------------------------------------------------------------------
vtkServiceEndpoint* vtkClientSession::GetEndpoint(ServiceTypes type) const
{
  const auto& internals = (*this->Internals);
  switch (type)
  {
    case DATA_SERVER:
      return internals.DSEndpoint;

    case RENDER_SERVER:
      return internals.RSEndpoint;

    default:
      return nullptr;
  }
}

//----------------------------------------------------------------------------
std::vector<vtkServiceEndpoint*> vtkClientSession::GetEndpoints(int type) const
{
  const auto& internals = (*this->Internals);
  return internals.GetEndpoints(type);
}

//----------------------------------------------------------------------------
rxcpp::observable<std::tuple<std::string, vtkRemoteObjectProvider::vtkProgressItem>>
vtkClientSession::GetProgressObservable() const
{
  const auto& internals = (*this->Internals);
  return internals.ProgressSubject.get_observable();
}

//----------------------------------------------------------------------------
bool vtkClientSession::GetIsRemote() const
{
  const auto& internals = (*this->Internals);
  return internals.IsRemote;
}

//----------------------------------------------------------------------------
void vtkClientSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
