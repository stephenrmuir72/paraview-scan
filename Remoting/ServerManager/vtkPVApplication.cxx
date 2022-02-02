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
  std::map<vtkTypeUInt32, SessionInfo> Sessions;
  vtkTypeUInt32 NextSessionID{ 1 };
};

vtkStandardNewMacro(vtkPVApplication);
//----------------------------------------------------------------------------
vtkPVApplication::vtkPVApplication()
  : vtkPVCoreApplication(vtkPVCoreApplication::UI)
  , Internals(new vtkPVApplication::vtkInternals())
{
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
vtkTypeUInt32 vtkPVApplication::CreateBuiltinSession()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  const auto id = internals.NextSessionID++;

  SessionInfo info;
  info.Server = vtk::TakeSmartPointer(vtkServerSession::New());
  info.Server->StartServices();

  info.Client = vtk::TakeSmartPointer(vtkClientSession::New());
  info.Client->Connect();
  internals.Sessions.emplace(id, std::move(info));
  return id;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVApplication::CreateRemoteSession(const std::string& url)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  const auto id = internals.NextSessionID++;

  SessionInfo info;
  info.Client = vtk::TakeSmartPointer(vtkClientSession::New());
  info.Client->Connect(url);
  internals.Sessions.emplace(id, std::move(info));
  return id;
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
