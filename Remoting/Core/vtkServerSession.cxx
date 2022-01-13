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
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkProcessModule.h"
#include "vtkRemoteObjectProvider.h"
#include "vtkService.h"
#include "vtkServicesEngine.h"

struct ServiceInfo
{
  vtkSmartPointer<vtkService> Service;
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkSmartPointer<vtkObjectStore> ObjectStore;
  vtkSmartPointer<vtkRemoteObjectProvider> RemoteObjectProvider;
  vtkSmartPointer<vtkInformationProvider> InformationProvider;
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
  info.Controller = pm->NewController();
  info.ObjectStore = vtk::TakeSmartPointer(vtkObjectStore::New());
  info.Service = engine->CreateService(name);

  // setup providers for the service.
  info.RemoteObjectProvider = vtk::TakeSmartPointer(vtkRemoteObjectProvider::New());
  info.RemoteObjectProvider->SetObjectStore(info.ObjectStore);
  info.RemoteObjectProvider->SetProxyDefinitionManager(self->GetProxyDefinitionManager());
  info.RemoteObjectProvider->SetController(info.Controller);
  info.RemoteObjectProvider->Initialize(info.Service);

  info.InformationProvider = vtk::TakeSmartPointer(vtkInformationProvider::New());
  info.InformationProvider->SetObjectStore(info.ObjectStore);
  info.InformationProvider->SetController(info.Controller);

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
