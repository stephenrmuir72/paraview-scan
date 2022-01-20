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

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkSMProxyManager.h"
#include "vtkServiceEndpoint.h"
#include "vtkServicesEngine.h"

#include <cassert>

class vtkClientSession::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkSmartPointer<vtkServiceEndpoint> DSEndpoint;
  vtkSmartPointer<vtkServiceEndpoint> RSEndpoint;
  vtkSmartPointer<vtkSMProxyManager> ProxyManager;

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
};

//----------------------------------------------------------------------------
void vtkClientSession::vtkInternals::InitializeSession(vtkClientSession* self)
{
  assert(this->DSEndpoint && this->RSEndpoint);
  this->ProxyManager = vtk::TakeSmartPointer(vtkSMProxyManager::New());
  this->ProxyManager->SetSession(self);
}

//============================================================================
vtkStandardNewMacro(vtkClientSession);
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

  auto* pm = vtkProcessModule::GetProcessModule();
  assert(pm != nullptr);
  auto* engine = pm->GetServicesEngine();
  if (pm->GetRank() == 0)
  {
    // FIXME: ASYNC
    // These should not "wait"
    internals.DSEndpoint = engine->CreateServiceEndpoint("ds", url);
    internals.DSEndpoint->Connect().Wait();

    internals.RSEndpoint = engine->CreateServiceEndpoint("rs", url);
    internals.RSEndpoint->Connect().Wait();

    // auto o2 = internals.RSEndpoint->Connect().GetObservable();
    // return o1.combine_latest(o2).observe_on(engine->GetCoordination())
    //   .map([](const std::tuple<bool, bool>& status){
    //       return std::get<0>(status) && std::get<1>(status);
    //   });
    internals.InitializeSession(this);
  }

  return rxcpp::sources::never<bool>();
}

//----------------------------------------------------------------------------
vtkSMProxyManager* vtkClientSession::GetProxyManager() const
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
      return CLIENT;

    case (CLIENT | RENDER_SERVER):
      return CLIENT;

    case (CLIENT | DATA_SERVER | RENDER_SERVER):
      return CLIENT;

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
  auto* pm = vtkProcessModule::GetProcessModule();
  return pm->GetServicesEngine()->GetCoordination();
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
void vtkClientSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
