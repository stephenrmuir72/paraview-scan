/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplication.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVApplication.h"

#include "vtkClientSession.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkServerSession.h"

#include <algorithm>
#include <map>

struct SessionInfo
{
  vtkSmartPointer<vtkClientSession> Client;
  vtkSmartPointer<vtkServerSession> Server;
};

class vtkPVApplication::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkPVApplication* Self{ nullptr };
  std::map<vtkTypeUInt32, SessionInfo> Sessions;
  vtkTypeUInt32 NextSessionID{ 1 };

  rxcpp::observable<vtkTypeUInt32> ClientConnect(const std::string& url, SessionInfo&& info)
  {
    auto id = this->NextSessionID++;
    info.Client = vtk::TakeSmartPointer(vtkClientSession::New());
    auto behavior = std::make_shared<rxcpp::subjects::behavior<vtkVariant>>(vtkVariant());
    info.Client->Connect(url).subscribe([info, this, id, behavior](bool status) {
      if (status)
      {
        this->Sessions.emplace(id, info);
        this->Self->InvokeEvent(
          vtkCommand::ConnectionCreatedEvent, const_cast<vtkTypeUInt32*>(&id));
      }
      behavior->get_subscriber().on_next(vtkVariant(status ? 1 : 0));
    });

    return behavior->get_observable()
      .filter([](const vtkVariant& variant) { return variant.IsValid(); })
      .take(1)
      .map([behavior, id](const vtkVariant& variant) {
        (void)behavior;
        return variant.ToInt() == 1 ? id : 0;
      });
  }
};

vtkStandardNewMacro(vtkPVApplication);
//----------------------------------------------------------------------------
vtkPVApplication::vtkPVApplication()
  : vtkPVCoreApplication(vtkPVCoreApplication::UI)
  , Internals(new vtkPVApplication::vtkInternals())
{
  this->Internals->Self = this;
}

//----------------------------------------------------------------------------
vtkPVApplication::~vtkPVApplication() = default;

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVApplication::GetInstance()
{
  return vtkPVApplication::SafeDownCast(vtkPVCoreApplication::GetInstance());
}

//----------------------------------------------------------------------------
bool vtkPVApplication::InitializeInternal()
{

  return true;
}

//----------------------------------------------------------------------------
void vtkPVApplication::CloseSession(vtkTypeUInt32 id)
{
  // fire connection closed event
  this->InvokeEvent(vtkCommand::ConnectionClosedEvent, &id);

  auto& internals = (*this->Internals);
  auto iter = internals.Sessions.find(id);
  if (iter != internals.Sessions.end())
  {
    internals.Sessions.erase(iter);
  }
}

//----------------------------------------------------------------------------
void vtkPVApplication::FinalizeInternal()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.Sessions.clear();
}

//----------------------------------------------------------------------------
rxcpp::observable<vtkTypeUInt32> vtkPVApplication::CreateBuiltinSession()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  SessionInfo info;
  info.Server = vtk::TakeSmartPointer(vtkServerSession::New());
  info.Server->StartServices();
  return internals.ClientConnect({}, std::move(info));
}

//----------------------------------------------------------------------------
rxcpp::observable<vtkTypeUInt32> vtkPVApplication::CreateRemoteSession(const std::string& url)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return internals.ClientConnect(url, {});
}

//----------------------------------------------------------------------------
vtkClientSession* vtkPVApplication::GetSession(vtkTypeUInt32 id) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  auto iter = internals.Sessions.find(id);
  return iter != internals.Sessions.end() ? iter->second.Client.GetPointer() : nullptr;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVApplication::GetSessionID(vtkClientSession* session) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  auto iter = std::find_if(internals.Sessions.begin(), internals.Sessions.end(),
    [session](const auto& pair) { return pair.second.Client == session; });
  return iter != internals.Sessions.end() ? iter->first : 0;
}

//----------------------------------------------------------------------------
void vtkPVApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
