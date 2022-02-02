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

#ifndef vtkSMSessionProxyManagerInternals_h
#define vtkSMSessionProxyManagerInternals_h

#include "vtkObjectBase.h"
#include "vtkSMLink.h"                // for vtkSMLink
#include "vtkSMProxyLocator.h"        // for vtkSMProxyLocator
#include "vtkSMProxySelectionModel.h" // for vtkSMProxySelectionModel
#include "vtkSmartPointer.h"          // for vtkSmartPointer

#include <map>                          // for std::map
#include <set>                          // for std::set
#include <vector>                       // for std::vector
#include <vtksys/RegularExpression.hxx> // for regexes

class vtkSMProxyLocator;
class vtkSMSessionProxyManager;

// Sub-classed to avoid symbol length explosion.
class vtkSMProxyManagerProxyInfo : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkSMProxyManagerProxyInfo, vtkObjectBase);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

  vtkSmartPointer<vtkSMProxy> Proxy;
  unsigned long ModifiedObserverTag;
  unsigned long StateChangedObserverTag;
  unsigned long UpdateObserverTag;
  unsigned long UpdateInformationObserverTag;

  static vtkSMProxyManagerProxyInfo* New()
  {
    // This is required everytime we're implementing our own New() to avoid
    // "Deleting unknown object" warning from vtkDebugLeaks.
    vtkSMProxyManagerProxyInfo* ret = new vtkSMProxyManagerProxyInfo();
    ret->InitializeObjectBase();
    return ret;
  }

  vtkSMProxyManagerProxyInfo(const vtkSMProxyManagerProxyInfo&) = delete;
  void operator=(const vtkSMProxyManagerProxyInfo&) = delete;

private:
  vtkSMProxyManagerProxyInfo()
  {
    this->ModifiedObserverTag = 0;
    this->StateChangedObserverTag = 0;
    this->UpdateObserverTag = 0;
    this->UpdateInformationObserverTag = 0;
  }
  ~vtkSMProxyManagerProxyInfo() override
  {
    // Remove observers.
    if (this->ModifiedObserverTag && this->Proxy.GetPointer())
    {
      this->Proxy.GetPointer()->RemoveObserver(this->ModifiedObserverTag);
      this->ModifiedObserverTag = 0;
    }
    if (this->StateChangedObserverTag && this->Proxy.GetPointer())
    {
      this->Proxy.GetPointer()->RemoveObserver(this->StateChangedObserverTag);
      this->StateChangedObserverTag = 0;
    }
    if (this->UpdateObserverTag && this->Proxy.GetPointer())
    {
      this->Proxy.GetPointer()->RemoveObserver(this->UpdateObserverTag);
      this->UpdateObserverTag = 0;
    }
    if (this->UpdateInformationObserverTag && this->Proxy.GetPointer())
    {
      this->Proxy.GetPointer()->RemoveObserver(this->UpdateInformationObserverTag);
      this->UpdateInformationObserverTag = 0;
    }
  }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyListType
  : public std::vector<vtkSmartPointer<vtkSMProxyManagerProxyInfo>>
{
public:
  // Returns if the proxy exists in  this vector.
  bool Contains(vtkSMProxy* proxy)
  {
    vtkSMProxyManagerProxyListType::iterator iter = this->begin();
    for (; iter != this->end(); ++iter)
    {
      if (iter->GetPointer()->Proxy == proxy)
      {
        return true;
      }
    }
    return false;
  }
  vtkSMProxyManagerProxyListType::iterator Find(vtkSMProxy* proxy)
  {
    vtkSMProxyManagerProxyListType::iterator iter = this->begin();
    for (; iter != this->end(); ++iter)
    {
      if (iter->GetPointer()->Proxy.GetPointer() == proxy)
      {
        return iter;
      }
    }
    return this->end();
  }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyMapType : public std::map<std::string, vtkSMProxyManagerProxyListType>
{
};
//-----------------------------------------------------------------------------
struct vtkSMProxyManagerEntry
{
  std::string Group;
  std::string Name;
  vtkSmartPointer<vtkSMProxy> Proxy;

  vtkSMProxyManagerEntry(const char* group, const char* name, vtkSMProxy* proxy)
  {
    this->Group = group;
    this->Name = name;
    this->Proxy = proxy;
  }

  bool operator==(const vtkSMProxyManagerEntry& other) const
  {
    return this->Group == other.Group && this->Name == other.Name &&
      this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID();
  }

  bool operator<(const vtkSMProxyManagerEntry& other) const
  {
    if (this->Proxy->GetGlobalID() < other.Proxy->GetGlobalID())
    {
      return true;
    }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID() && this->Group == other.Group)
    {
      return this->Name < other.Name;
    }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID())
    {
      return this->Group < other.Group;
    }
    return false;
  }

  bool operator>(const vtkSMProxyManagerEntry& other) const
  {
    if (this->Proxy->GetGlobalID() > other.Proxy->GetGlobalID())
    {
      return true;
    }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID() && this->Group == other.Group)
    {
      return this->Name > other.Name;
    }
    else if (this->Proxy->GetGlobalID() == other.Proxy->GetGlobalID())
    {
      return this->Group > other.Group;
    }
    return false;
  }
};
//-----------------------------------------------------------------------------
struct vtkSMSessionProxyManagerInternals
{
  // This data structure stores actual proxy instances grouped in
  // collections.
  typedef std::map<std::string, vtkSMProxyManagerProxyMapType> ProxyGroupType;
  ProxyGroupType RegisteredProxyMap;

  // This data structure stores the tuples(group, name, proxy) to compute
  // diff when a state is loaded.
  typedef std::set<vtkSMProxyManagerEntry> GroupNameProxySet;
  GroupNameProxySet RegisteredProxyTuple;

  // This data structure stores a set of proxies that have been modified.
  typedef std::set<vtkSMProxy*> SetOfProxies;
  SetOfProxies ModifiedProxies;

  // Data structure to save registered links.
  typedef std::map<std::string, vtkSmartPointer<vtkSMLink>> LinkType;
  LinkType RegisteredLinkMap;

  // Data structure for selection models.
  typedef std::map<std::string, vtkSmartPointer<vtkSMProxySelectionModel>> SelectionModelsType;
  SelectionModelsType SelectionModels;

  // Keep ref to the proxyManager to access the session
  vtkSMSessionProxyManager* ProxyManager;

  vtkTypeUInt32 NextGlobalID{ 100 };

  // Helper methods -----------------------------------------------------------
  void FindProxyTuples(vtkSMProxy* proxy, std::set<vtkSMProxyManagerEntry>& tuplesFounds)
  {
    std::set<vtkSMProxyManagerEntry>::iterator iter;
    iter = this->RegisteredProxyTuple.begin();
    while (iter != this->RegisteredProxyTuple.end())
    {
      if (iter->Proxy == proxy)
      {
        tuplesFounds.insert(*iter);
      }
      iter++;
    }
  }

  // --------------------------------------------------------------------------
  void RemoveTuples(const char* name, std::set<vtkSMProxyManagerEntry>& removedEntries)
  {
    // Convert param
    std::string nameString = name;

    // Deal with set
    GroupNameProxySet resultSet;
    std::set<vtkSMProxyManagerEntry>::iterator iter;
    iter = this->RegisteredProxyTuple.begin();
    while (iter != this->RegisteredProxyTuple.end())
    {
      if (iter->Name != nameString)
      {
        resultSet.insert(*iter);
      }
      iter++;
    }
    this->RegisteredProxyTuple = resultSet;

    // Deal with map only (true)
    vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
      this->RegisteredProxyMap.begin();
    for (; it != this->RegisteredProxyMap.end(); it++)
    {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
      {
        this->RemoveTuples(it->first.c_str(), name, removedEntries, true);
      }
    }
  }

  // --------------------------------------------------------------------------
  void RemoveTuples(const char* group, const char* name,
    std::set<vtkSMProxyManagerEntry>& removedEntries, bool doMapOnly)
  {
    // Convert parameters
    std::string groupString = group;
    std::string nameString = name;

    // Deal with set
    if (!doMapOnly)
    {
      GroupNameProxySet resultSet;
      std::set<vtkSMProxyManagerEntry>::iterator iter;
      iter = this->RegisteredProxyTuple.begin();
      while (iter != this->RegisteredProxyTuple.end())
      {
        if (iter->Group != groupString || iter->Name != nameString)
        {
          resultSet.insert(*iter);
        }
        iter++;
      }
      this->RegisteredProxyTuple = resultSet;
    }

    // Deal with map
    vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
      this->RegisteredProxyMap.find(group);
    if (it != this->RegisteredProxyMap.end())
    {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
      {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
        while (it3 != it2->second.end())
        {
          removedEntries.insert(vtkSMProxyManagerEntry(group, name, (*it3)->Proxy));
          it3++;
        }
        it->second.erase(it2);
      }
    }
  }

  // --------------------------------------------------------------------------
  // Return true if the given tuple has been found
  bool RemoveTuples(const char* group, const char* name, vtkSMProxy* proxy)
  {
    // Convert parameters
    std::string groupString = group;
    std::string nameString = name;

    // Will be overridden if the proxy is found
    bool found = false;

    // Deal with set
    this->RegisteredProxyTuple.erase(vtkSMProxyManagerEntry(group, name, proxy));

    // Deal with map
    vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
      this->RegisteredProxyMap.find(group);
    if (it != this->RegisteredProxyMap.end())
    {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
      {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.Find(proxy);
        if (it3 != it2->second.end())
        {
          found = true;
          it2->second.erase(it3);
        }
        if (it2->second.empty())
        {
          it->second.erase(it2);
        }
      }
    }


    return found;
  }
};

#endif

// VTK-HeaderTest-Exclude: vtkSMSessionProxyManagerInternals.h
