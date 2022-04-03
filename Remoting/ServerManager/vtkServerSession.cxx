/*=========================================================================

  Program:   ParaView
  Module:    vtkServerSession.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkServerSession.h"

#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkPVCoreApplication.h"
#include "vtkRemoteObjectProvider.h"
#include "vtkService.h"
#include "vtkServicesEngine.h"

struct ServiceInfo
{
  vtkSmartPointer<vtkService> Service;
  vtkSmartPointer<vtkObjectStore> ObjectStore;
  std::vector<vtkSmartPointer<vtkProvider>> Providers;
};

class vtkServerSession::vtkInternals
{
  std::map<std::string, ServiceInfo> Services;
public:
  void StartService(const std::string& name, vtkServerSession* self);

  ~vtkInternals()
  {
    for (auto& pair : this->Services)
    {
      if (pair.second.Service)
      {
        auto* engine = pair.second.Service->GetEngine();
        engine->UnRegisterService(pair.second.Service);
      }
    }
  }
};

//----------------------------------------------------------------------------
void vtkServerSession::vtkInternals::StartService(const std::string& name, vtkServerSession* self)
{
  auto* pvapp = vtkPVCoreApplication::GetInstance();
  assert(pvapp != nullptr);
  auto* engine = pvapp->GetServicesEngine();

  ServiceInfo info;
  info.ObjectStore = vtk::TakeSmartPointer(vtkObjectStore::New());
  info.Service = engine->CreateService(name);

  auto remoteObjectProvider = vtk::TakeSmartPointer(vtkRemoteObjectProvider::New());
  remoteObjectProvider->SetObjectStore(info.ObjectStore);
  remoteObjectProvider->SetProxyDefinitionManager(self->GetProxyDefinitionManager());
  remoteObjectProvider->Initialize(info.Service);
  info.Providers.push_back(remoteObjectProvider);

  // let sub-classes initialize services.
  self->InitializeService(info.Service, info.ObjectStore);

  this->Services.emplace(name, info);
  info.Service->Start();
}

//============================================================================
vtkObjectFactoryNewMacro(vtkServerSession);
//----------------------------------------------------------------------------
vtkServerSession::vtkServerSession()
  : Internals(new vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkServerSession::~vtkServerSession() = default;

//----------------------------------------------------------------------------
void vtkServerSession::StartServices()
{
  auto& internals = (*this->Internals);
  internals.StartService("ds", this);
  internals.StartService("rs", this);
}

//----------------------------------------------------------------------------
void vtkServerSession::InitializeService(vtkService*, vtkObjectStore*) {}

//----------------------------------------------------------------------------
void vtkServerSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
