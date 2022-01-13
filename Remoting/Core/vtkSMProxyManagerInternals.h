/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManagerInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMProxyManagerInternals
 * @brief
 *
 */

#ifndef vtkSMProxyManagerInternals_h
#define vtkSMProxyManagerInternals_h

#include "vtkChannelSubscription.h"
#include "vtkClientSession.h"
#include "vtkLogger.h"
#include "vtkRemoteObjectProvider.h"
#include "vtkRemotingCoreModule.h"
#include "vtkSMProxy.h"
#include "vtkServiceEndpoint.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>

class vtkSMProxyManager::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkSMProxyManager* Self{ nullptr };
  vtkWeakPointer<vtkClientSession> Session;
  vtkTypeUInt32 NextGlobalID{ 100 };

  std::vector<vtkSmartPointer<vtkChannelSubscription>> Subscriptions;

  std::map<vtkTypeUInt32, vtkWeakPointer<vtkSMProxy>> Proxies;

  std::multimap<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>> RegisteredProxies;

  std::map<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>> Prototypes;
  static auto GetKey(const std::string& group, const std::string& name)
  {
    return std::make_pair(group, name);
  }

  void Subscribe(vtkServiceEndpoint* endpoint)
  {
    auto subscription = endpoint->Subscribe(vtkRemoteObjectProvider::CHANNEL_NOTIFY_UPDATE());
    subscription->GetObservable().subscribe([this](const vtkPacket& packet) {
      auto gids = vtkRemoteObjectProvider::ParseNotifyUpdate(packet);
      this->ProxiesUpdated(gids);
    });
    this->Subscriptions.push_back(subscription);
  }

private:
  /**
   * This is called whenever a service notifies us that an algorithm
   * has updated. The service buffers these notifications and sends
   * a list of gids for all algorithms that were updated.
   */
  void ProxiesUpdated(const std::vector<vtkTypeUInt32>& gids) const
  {
    for (auto id : gids)
    {
      auto iter = this->Proxies.find(id);
      if (iter != this->Proxies.end() && iter->second)
      {
        this->Self->MarkPipelineUpdated(iter->second);
      }
    }
  }
};

#endif
