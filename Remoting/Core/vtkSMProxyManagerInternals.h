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

#include "vtkClientSession.h"
#include "vtkRemotingCoreModule.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>

class vtkSMProxyManager::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkWeakPointer<vtkClientSession> Session;
  vtkTypeUInt32 NextGlobalID{ 100 };

  std::map<vtkTypeUInt32, vtkWeakPointer<vtkSMProxy>> Proxies;

  std::multimap<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>> RegisteredProxies;

  std::map<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>> Prototypes;
  static auto GetKey(const std::string& group, const std::string& name)
  {
    return std::make_pair(group, name);
  }
};

#endif
