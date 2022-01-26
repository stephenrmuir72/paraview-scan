/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessInitializer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessInitializer.h"

#include "paraview_reflection.h"
#include "vtkChannelSubscription.h"
#include "vtkClientSession.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkService.h"
#include "vtkServiceEndpoint.h"

#if VTK_MODULE_ENABLE_ParaView_ServerManagerViews
#include "vtkInteractionProvider.h"
#include "vtkPVViewProvider.h"
#include "vtkSMViewProxy.h"
#endif

vtkStandardNewMacro(vtkProcessInitializer);
//----------------------------------------------------------------------------
vtkProcessInitializer::vtkProcessInitializer() = default;

//----------------------------------------------------------------------------
vtkProcessInitializer::~vtkProcessInitializer() = default;

//----------------------------------------------------------------------------
void vtkProcessInitializer::InitializeReflection()
{
  paraview_reflection_initialize();
}

//----------------------------------------------------------------------------
void vtkProcessInitializer::InitializeService(vtkService* service, vtkObjectStore* store)
{
  this->Superclass::InitializeService(service, store);
#if VTK_MODULE_ENABLE_ParaView_ServerManagerViews
  if (service->GetName() == "rs")
  {
    auto provider = vtk::TakeSmartPointer(vtkInteractionProvider::New());
    provider->SetObjectStore(store);
    provider->Initialize(service);
    this->RegisterArtifact(service, provider);
  }

  auto viewProvider = vtk::TakeSmartPointer(vtkPVViewProvider::New());
  viewProvider->SetObjectStore(store);
  viewProvider->Initialize(service);
  this->RegisterArtifact(service, viewProvider);
#endif
}

//----------------------------------------------------------------------------
void vtkProcessInitializer::InitializeServiceEndpoint(
  vtkServiceEndpoint* endpoint, vtkSession* session)
{
  this->Superclass::InitializeServiceEndpoint(endpoint, session);
#if VTK_MODULE_ENABLE_ParaView_ServerManagerViews
  if (endpoint->GetServiceName() == "rs")
  {
    auto* clientSession = vtkClientSession::SafeDownCast(session);
    auto* pxm = clientSession->GetProxyManager();
    auto cSubscription = endpoint->Subscribe(vtkPVViewProvider::GetImageChannelName());
    cSubscription->GetObservable().subscribe([pxm](const vtkPacket& packet) {
      const auto& json = packet.GetJSON();
      const auto gid = json.at("gid").get<vtkTypeUInt32>();
      auto proxy = pxm->FindProxy(gid);
      if (auto* view = vtkSMViewProxy::SafeDownCast(proxy))
      {
        view->PostRenderingResult(packet);
      }
    });
    this->RegisterArtifact(endpoint, cSubscription);
  }
#endif
}
