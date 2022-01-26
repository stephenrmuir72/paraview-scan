/*=========================================================================

  Program:   ParaView
  Module:    vtkClientSessionViews.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientSessionViews.h"

#include "vtkChannelSubscription.h"
#include "vtkObjectFactory.h"
#include "vtkPVViewProvider.h"
#include "vtkPacket.h"
#include "vtkSMProxyManager.h"
#include "vtkSMViewProxy.h"
#include "vtkServiceEndpoint.h"
#include "vtkSmartPointer.h"

class vtkClientSessionViews::vtkInternals
{
public:
  std::vector<vtkSmartPointer<vtkChannelSubscription>> Subscriptions;
};

vtkObjectFactoryNewMacro(vtkClientSessionViews);
//----------------------------------------------------------------------------
vtkClientSessionViews::vtkClientSessionViews()
  : Internals(new vtkClientSessionViews::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkClientSessionViews::~vtkClientSessionViews() = default;

//----------------------------------------------------------------------------
void vtkClientSessionViews::InitializeServiceEndpoint(vtkServiceEndpoint* endpoint)
{
  this->Superclass::InitializeServiceEndpoint(endpoint);

  if (endpoint->GetServiceName() == "rs")
  {
    auto* pxm = this->GetProxyManager();
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

    this->Internals->Subscriptions.push_back(cSubscription);
  }
}

//----------------------------------------------------------------------------
void vtkClientSessionViews::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
