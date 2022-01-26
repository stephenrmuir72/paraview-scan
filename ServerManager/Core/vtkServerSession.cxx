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

#include "vtkInformationProvider.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkProcessModule.h"
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
};

//----------------------------------------------------------------------------
void vtkServerSession::vtkInternals::StartService(const std::string& name, vtkServerSession* self)
{
  auto* pm = vtkProcessModule::GetProcessModule();
  assert(pm != nullptr);
  auto* engine = pm->GetServicesEngine();

  ServiceInfo info;
  info.ObjectStore = vtk::TakeSmartPointer(vtkObjectStore::New());
  info.Service = engine->CreateService(name);

  auto remoteObjectProvider = vtk::TakeSmartPointer(vtkRemoteObjectProvider::New());
  remoteObjectProvider->SetObjectStore(info.ObjectStore);
  remoteObjectProvider->SetProxyDefinitionManager(self->GetProxyDefinitionManager());
  remoteObjectProvider->Initialize(info.Service);

  auto informationProvider = vtk::TakeSmartPointer(vtkInformationProvider::New());
  informationProvider->SetObjectStore(info.ObjectStore);
  informationProvider->Initialize(info.Service);
  info.Providers.push_back(remoteObjectProvider);
  info.Providers.push_back(informationProvider);

  // Let the vtkProcessModule init the service.
  pm->InitializeService(info.Service, info.ObjectStore);

  this->Services.emplace(name, info);
  info.Service->Start();
}

//============================================================================
vtkStandardNewMacro(vtkServerSession);
//----------------------------------------------------------------------------
vtkServerSession::vtkServerSession()
  : Internals(new vtkInternals())
{
  auto& internals = (*this->Internals);
  internals.StartService("ds", this);
  internals.StartService("rs", this);
}

//----------------------------------------------------------------------------
vtkServerSession::~vtkServerSession() = default;

//----------------------------------------------------------------------------
void vtkServerSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
