/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSessionProxyManagerInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMSessionProxyManagerInternals_h
#define vtkSMSessionProxyManagerInternals_h

#include "vtkClientSession.h" // for vtkClientSession
#include "vtkCommand.h"       // for vtkCommand
#include "vtkNew.h"           // for vtkNew
#include "vtkObjectBase.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkSMLink.h"                // for vtkSMLink
#include "vtkSMProxyLocator.h"        // for vtkSMProxyLocator
#include "vtkSMProxyManager.h"        // for vtkSMProxyManager
#include "vtkSMProxySelectionModel.h" // for vtkSMProxySelectionModel
#include "vtkSmartPointer.h"          // for vtkSmartPointer
#include "vtkWeakPointer.h"           // for vtkWeakPointer

#include <map>                          // for std::map
#include <set>                          // for std::set
#include <unordered_map>                // for std::unordered_map
#include <vector>                       // for std::vector
#include <vtksys/RegularExpression.hxx> // for regexes

class vtkSMProxyLocator;
class vtkSMSessionProxyManager;

//-----------------------------------------------------------------------------
class vtkSMProxyManagerObserver : public vtkCommand
{
public:
  static vtkSMProxyManagerObserver* New() { return new vtkSMProxyManagerObserver(); }
  void SetTarget(vtkSMSessionProxyManager* t) { this->Target = t; }
  void Execute(vtkObject* obj, unsigned long event, void* data) override
  {
    if (this->Target)
    {
      this->Target->ExecuteEvent(obj, event, data);
    }
  }

protected:
  vtkSMProxyManagerObserver()
    : Target(nullptr)
  {
  }
  vtkSMSessionProxyManager* Target;
};


//-----------------------------------------------------------------------------
struct vtkSMSessionProxyManagerInternals
{
  const std::thread::id OwnerTID{ std::this_thread::get_id() };

  // Registered proxies
  std::multimap<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>> RegisteredProxies;

  // All created proxies (except prototype proxies)
  std::unordered_map<vtkTypeUInt32, vtkWeakPointer<vtkSMProxy>> Proxies;

  // Registered links.
  std::map<std::string, vtkSmartPointer<vtkSMLink>> RegisteredLinks;

  // Data structure for selection models.
  typedef std::map<std::string, vtkSmartPointer<vtkSMProxySelectionModel>> SelectionModelsType;
  SelectionModelsType SelectionModels;

  // Keep ref to the proxyManager to access the session
  vtkSMSessionProxyManager* ProxyManager;

  vtkNew<vtkSMProxyManagerObserver> Observer;
  vtkWeakPointer<vtkClientSession> Session;
  vtkTypeUInt32 NextGlobalID{ 100 };

  vtkSMSessionProxyManagerInternals(vtkSMSessionProxyManager* self)
    : ProxyManager(self)
  {
    this->Observer->SetTarget(self);
  }

  void SetSession(vtkClientSession* session)
  {
    this->Session = session;
    // TODO:
  }

  static auto GetKey(const std::string& group, const std::string& name)
  {
    return std::make_pair(group, name);
  }

  static const std::string& GetGroup(const std::pair<std::string, std::string>& key)
  {
    return key.first;
  }

  static const std::string& GetName(const std::pair<std::string, std::string>& key)
  {
    return key.second;
  }
};

#endif

// VTK-HeaderTest-Exclude: vtkSMSessionProxyManagerInternals.h
