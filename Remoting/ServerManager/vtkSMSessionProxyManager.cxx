/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSessionProxyManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSessionProxyManager.h"

#include "vtkClientSession.h"
#include "vtkCollection.h"
#include "vtkDebugLeaks.h"
#include "vtkEventForwarderCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVCoreApplication.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProxyDefinitionManager.h"
#include "vtkReflection.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMDocumentation.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxySelectionModel.h"
// #include "vtkSMSettingsProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"
#include "vtkVersion.h"

#include "vtksys/FStream.hxx"
#include "vtksys/RegularExpression.hxx"

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "vtkSMSessionProxyManagerInternals.h"

#if 0 // for debugging
class vtkSMProxyRegObserver : public vtkCommand
{
public:
  virtual void Execute(vtkObject*, unsigned long event, void* data)
    {
      vtkSMProxyManager::RegisteredProxyInformation* info =
        (vtkSMProxyManager::RegisteredProxyInformation*)data;
      cout << info->Proxy
           << " " << vtkCommand::GetStringFromEventId(event)
           << " " << info->GroupName
           << " " << info->ProxyName
           << endl;
    }
};
#endif

class vtkSMProxyManagerProxySet : public std::set<vtkSMProxy*>
{
};

//*****************************************************************************
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
  vtkSMProxyManagerObserver() { this->Target = nullptr; }
  vtkSMSessionProxyManager* Target;
};
//*****************************************************************************
class vtkSMProxyManagerForwarder : public vtkCommand
{
public:
  static vtkSMProxyManagerForwarder* New() { return new vtkSMProxyManagerForwarder(); }

  void Execute(vtkObject*, unsigned long event, void* data) override
  {
    //    if (vtkSMProxyManager::IsInitialized())
    //    {
    //      vtkSMProxyManager::GetProxyManager()->InvokeEvent(event, data);
    //    }
  }

protected:
  vtkSMProxyManagerForwarder() = default;
};
//*****************************************************************************
vtkStandardNewMacro(vtkSMSessionProxyManager);

//---------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMSessionProxyManager::New(vtkClientSession* session)
{
  assert(session != nullptr);
  auto* pxm = vtkSMSessionProxyManager::New();
  pxm->Session = session;
  return pxm;
}

//---------------------------------------------------------------------------
vtkSMSessionProxyManager::vtkSMSessionProxyManager()
{
  this->StateUpdateNotification = true;
  this->UpdateInputProxies = 0;
  this->InLoadXMLState = false;

  this->Internals = new vtkSMSessionProxyManagerInternals;
  this->Internals->ProxyManager = this;

  this->Observer = vtkSMProxyManagerObserver::New();
  this->Observer->SetTarget(this);
#if 0 // for debugging
  vtkSMProxyRegObserver* obs = new vtkSMProxyRegObserver;
  this->AddObserver(vtkCommand::RegisterEvent, obs);
  this->AddObserver(vtkCommand::UnRegisterEvent, obs);
#endif

  // setup event forwarder so that it forwards all events fired by this class via
  // the global proxy manager.
  vtkNew<vtkSMProxyManagerForwarder> forwarder;
  this->AddObserver(vtkCommand::AnyEvent, forwarder.GetPointer());
}

//---------------------------------------------------------------------------
vtkSMSessionProxyManager::~vtkSMSessionProxyManager()
{
  // This is causing a PushState() when the object is being destroyed. This
  // causes errors since the ProxyManager is destroyed only when the session is
  // being deleted, thus the session cannot be valid at this point.
  // this->UnRegisterProxies();
  delete this->Internals;

  this->Observer->SetTarget(nullptr);
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
void vtkSMSessionProxyManager::InstantiateGroupPrototypes(const char* groupName)
{
  auto* pdm = this->GetProxyDefinitionManager();
  assert(pdm != 0);

  // Find the XML elements from which the proxies can be instantiated and
  // initialized
  for (const auto& item : pdm->GetDefinitions(groupName ? groupName : ""))
  {
    const auto group = item.GetGroup();
    const auto name = item.GetName();
    const auto regGroup = group + "_prototypes";
    if (this->GetProxy(regGroup.c_str(), name.c_str()) == nullptr)
    {
      vtkSMProxy* proxy = this->NewProxy(group.c_str(), name.c_str());
      if (proxy)
      {
        proxy->GlobalID = 0;
        this->RegisterProxy(regGroup.c_str(), name.c_str(), proxy);
        proxy->FastDelete();
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMSessionProxyManager::InstantiatePrototypes()
{
  this->InstantiateGroupPrototypes(nullptr);
}

//----------------------------------------------------------------------------
void vtkSMSessionProxyManager::ClearPrototypes()
{
  vtksys::RegularExpression prototypesRe("_prototypes$");

  // clear items from RegisteredProxyMap.
  for (auto group_iter = this->Internals->RegisteredProxyMap.begin();
       group_iter != this->Internals->RegisteredProxyMap.end();)
  {
    const bool isPrototypeGroup = prototypesRe.find(group_iter->first);
    if (isPrototypeGroup)
    {
      group_iter = this->Internals->RegisteredProxyMap.erase(group_iter);
    }
    else
    {
      ++group_iter;
    }
  }

  // now, also clear item from RegisteredProxyTuple.
  for (auto tuple_iter = this->Internals->RegisteredProxyTuple.begin();
       tuple_iter != this->Internals->RegisteredProxyTuple.end();)
  {
    const bool isPrototypeGroup = prototypesRe.find(tuple_iter->Group);
    if (isPrototypeGroup)
    {
      tuple_iter = this->Internals->RegisteredProxyTuple.erase(tuple_iter);
    }
    else
    {
      ++tuple_iter;
    }
  }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::NewProxy(
  const char* groupName, const char* proxyName, const char* subProxyName)
{
  if (!groupName || !proxyName)
  {
    return nullptr;
  }
  // Find the XML element from which the proxy can be instantiated and
  // initialized
  auto element = this->GetProxyElement(groupName, proxyName, subProxyName);

  // Support for secondary group
  std::string originalGroupName = groupName;
  if (element)
  {
    std::string tmpGroupName = element->GetAttributeOrEmpty("group");
    if (!tmpGroupName.empty())
    {
      element = this->GetProxyElement(tmpGroupName.c_str(), proxyName, subProxyName);
      originalGroupName = tmpGroupName;
    }
  }

  if (element)
  {
    return this->NewProxy(element, originalGroupName.c_str(), proxyName, subProxyName);
  }

  return nullptr;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::NewProxy(
  vtkPVXMLElement* pelement, const char* groupname, const char* proxyname, const char* subProxyName)
{
  std::ostringstream cname;
  cname << "vtkSM" << pelement->GetName();
  auto proxy = vtkReflection::CreateInstance<vtkSMProxy>(cname.str());
  if (proxy)
  {
    // XMLName/XMLGroup should be set before ReadXMLAttributes so sub proxy
    // can be found based on their names when sent to the PM Side
    proxy->ProxyManager = this;
    proxy->GlobalID = this->Internals->NextGlobalID++;
    proxy->SetXMLGroup(groupname);
    proxy->SetXMLName(proxyname);
    proxy->ReadXMLAttributes(this, pelement);
    proxy->Register(this);
    return proxy;
  }
  vtkWarningMacro("Creation of new proxy " << cname.str() << " failed (" << groupname << ", "
                                           << proxyname << ").");
  return nullptr;
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSessionProxyManager::GetProxyDocumentation(
  const char* groupName, const char* proxyName)
{
  if (!groupName || !proxyName)
  {
    return nullptr;
  }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  return proxy ? proxy->GetDocumentation() : nullptr;
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSessionProxyManager::GetPropertyDocumentation(
  const char* groupName, const char* proxyName, const char* propertyName)
{
  if (!groupName || !proxyName || !propertyName)
  {
    return nullptr;
  }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  if (proxy)
  {
    vtkSMProperty* prop = proxy->GetProperty(propertyName);
    if (prop)
    {
      return prop->GetDocumentation();
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkPVXMLElement> vtkSMSessionProxyManager::GetProxyElement(
  const char* groupName, const char* proxyName, const char* subProxyName)
{
  // FIXME: ASYNC
  // return this->ProxyDefinitionManager->GetCollapsedProxyDefinition(
  //   groupName, proxyName, subProxyName, true);
  //
  // "subProxyName" is no longer supported. we will no longer supported adding a
  // complete definition as a "SubProxy"; one must always add the definition
  // separately.
  return this->GetProxyDefinitionManager()->FindProxyLegacy(groupName, proxyName);
}

//---------------------------------------------------------------------------
unsigned int vtkSMSessionProxyManager::GetNumberOfProxies(const char* group)
{
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if (it != this->Internals->RegisteredProxyMap.end())
  {
    size_t size = 0;
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); ++it2)
    {
      size += it2->second.size();
    }
    return static_cast<unsigned int>(size);
  }
  return 0;
}

//---------------------------------------------------------------------------
// No errors are raised if a proxy definition for the requested proxy is not
// found.
vtkSMProxy* vtkSMSessionProxyManager::GetPrototypeProxy(const char* groupname, const char* name)
{
  std::string protype_group = groupname;
  protype_group += "_prototypes";
  vtkSMProxy* proxy = this->GetProxy(protype_group.c_str(), name);
  if (proxy)
  {
    return proxy;
  }

  // silently ask for the definition. If not found return nullptr.
  auto xmlElement = this->GetProxyElement(groupname, name);
  if (xmlElement == nullptr)
  {
    // No definition was located for the requested proxy.
    // Cannot create the prototype.
    return nullptr;
  }

  proxy = this->NewProxy(groupname, name);
  if (!proxy)
  {
    return nullptr;
  }
  // unset global id to make the proxy be treated as a prototype
  proxy->GlobalID = 0;
  // register the proxy as a prototype.
  this->RegisterProxy(protype_group.c_str(), name, proxy);
  proxy->Delete();
  return proxy;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RemovePrototype(const char* groupname, const char* proxyname)
{
  std::string prototype_group = groupname;
  prototype_group += "_prototypes";
  vtkSMProxy* proxy = this->GetProxy(prototype_group.c_str(), proxyname);
  if (proxy)
  {
    // prototype exists, so remove it.
    this->UnRegisterProxy(prototype_group.c_str(), proxyname, proxy);
  }
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::GetProxy(const char* group, const char* name)
{
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if (it != this->Internals->RegisteredProxyMap.end())
  {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
    if (it2 != it->second.end())
    {
      if (it2->second.begin() != it2->second.end())
      {
        return it2->second.front()->Proxy.GetPointer();
      }
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::GetProxy(const char* name)
{
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
  {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
    if (it2 != it->second.end())
    {
      if (it2->second.begin() != it2->second.end())
      {
        return it2->second.front()->Proxy.GetPointer();
      }
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::GetProxies(
  const char* group, const char* name, vtkCollection* collection)
{
  collection->RemoveAllItems();
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(group);
  if (it != this->Internals->RegisteredProxyMap.end())
  {
    if (name == nullptr)
    {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
      std::set<vtkTypeUInt32> ids;
      for (; it2 != it->second.end(); it2++)
      {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
        for (; it3 != it2->second.end(); ++it3)
        {
          if (ids.find(it3->GetPointer()->Proxy->GetGlobalID()) == ids.end())
          {
            ids.insert(it3->GetPointer()->Proxy->GetGlobalID());
            collection->AddItem(it3->GetPointer()->Proxy);
          }
        }
      }
    }
    else
    {
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.find(name);
      if (it2 != it->second.end())
      {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
        for (; it3 != it2->second.end(); ++it3)
        {
          collection->AddItem(it3->GetPointer()->Proxy);
        }
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::GetProxyNames(
  const char* groupname, vtkSMProxy* proxy, vtkStringList* names)
{
  if (!names)
  {
    return;
  }
  names->RemoveAllItems();

  if (!groupname || !proxy)
  {
    return;
  }

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if (it != this->Internals->RegisteredProxyMap.end())
  {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
    {
      if (it2->second.Contains(proxy))
      {
        names->AddString(it2->first.c_str());
      }
    }
  }
}

//---------------------------------------------------------------------------
std::string vtkSMSessionProxyManager::RegisterProxy(const char* groupname, vtkSMProxy* proxy)
{
  assert(proxy != nullptr);

  std::string label = vtkSMCoreUtilities::SanitizeName(proxy->GetXMLLabel());
  std::string name = this->GetUniqueProxyName(groupname, label.c_str());
  this->RegisterProxy(groupname, name.c_str(), proxy);
  return name;
}

//---------------------------------------------------------------------------
std::string vtkSMSessionProxyManager::GetUniqueProxyName(
  const char* groupname, const char* prefix, bool alwaysAppend)
{
  if (!groupname || !prefix)
  {
    return std::string();
  }

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if (it == this->Internals->RegisteredProxyMap.end())
  {
    std::ostringstream name_stream;
    name_stream << prefix;
    if (alwaysAppend)
    {
      int suffix = 1;
      name_stream << suffix;
    }
    return name_stream.str();
  }

  std::set<std::string> existingNames;

  for (vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin(); it2 != it->second.end();
       it2++)
  {
    existingNames.insert(it2->first);
  }

  if (!alwaysAppend)
  {
    if (existingNames.find(prefix) == existingNames.end())
    {
      return prefix;
    }
  }

  for (int suffix = 1; suffix < VTK_INT_MAX; ++suffix)
  {
    std::ostringstream name_stream;
    name_stream << prefix << suffix;
    if (existingNames.find(name_stream.str()) == existingNames.end())
    {
      return name_stream.str();
    }
  }

  vtkErrorMacro("Failed to come up with a unique name!");
  abort();
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetProxyName(const char* groupname, vtkSMProxy* proxy)
{
  if (!groupname || !proxy)
  {
    return nullptr;
  }

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if (it != this->Internals->RegisteredProxyMap.end())
  {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
    {
      if (it2->second.Contains(proxy))
      {
        return it2->first.c_str();
      }
    }
  }

  return nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetProxyName(const char* groupname, unsigned int idx)
{
  if (!groupname)
  {
    return nullptr;
  }

  unsigned int counter = 0;

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if (it != this->Internals->RegisteredProxyMap.end())
  {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
    {
      if (idx < counter + it2->second.size())
      {
        // idx is between counter and the size of the next vector of
        // proxies, so return the current proxy map key
        return it2->first.c_str();
      }

      counter += static_cast<unsigned int>(it2->second.size());
    }
  }

  return nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::IsProxyInGroup(vtkSMProxy* proxy, const char* groupname)
{
  if (!proxy || !groupname)
  {
    return nullptr;
  }
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if (it != this->Internals->RegisteredProxyMap.end())
  {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
    {
      if (it2->second.Contains(proxy))
      {
        return it2->first.c_str();
      }
    }
  }
  return nullptr;
}

namespace
{
struct vtkSMProxyManagerProxyInformation
{
  std::string GroupName;
  std::string ProxyName;
  vtkSMProxy* Proxy;
};
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxies()
{
  // Clear internal proxy containers
  std::vector<vtkSMProxyManagerProxyInformation> toUnRegister;
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetModeToAll();
  iter->SetSessionProxyManager(this);
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProxyManagerProxyInformation info;
    info.GroupName = iter->GetGroup();
    info.ProxyName = iter->GetKey();
    info.Proxy = iter->GetProxy();
    toUnRegister.push_back(info);
  }
  iter->Delete();

  std::vector<vtkSMProxyManagerProxyInformation>::iterator vIter = toUnRegister.begin();
  for (; vIter != toUnRegister.end(); ++vIter)
  {
    this->UnRegisterProxy(vIter->GroupName.c_str(), vIter->ProxyName.c_str(), vIter->Proxy);
  }

  this->Internals->ModifiedProxies.clear();
  this->Internals->RegisteredProxyTuple.clear();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxy(
  const char* group, const char* name, vtkSMProxy* proxy)
{
  if (!group || !name)
  {
    return;
  }

  // Just in case a proxy ref is NOT held outside the ProxyManager iteself
  // Keep one during the full method call so the event could still have a valid
  // proxy object.
  vtkSmartPointer<vtkSMProxy> proxyHolder = proxy;
  std::string nameHolder(name);
  std::string groupHolder(group);

  // Do something only if the given tuple was found
  if (this->Internals->RemoveTuples(group, name, proxy))
  {
    vtkSMSessionProxyManager::RegisteredProxyInformation info;
    info.Proxy = proxy;
    info.GroupName = groupHolder.c_str();
    info.ProxyName = nameHolder.c_str();
    info.Type = vtkSMSessionProxyManager::RegisteredProxyInformation::PROXY;

    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    this->UnMarkProxyAsModified(info.Proxy);
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxy(const char* name)
{
  // Remove entries and keep them in a set
  std::set<vtkSMProxyManagerEntry> entriesToRemove;
  this->Internals->RemoveTuples(name, entriesToRemove);

  // Notify that some entries have been deleted
  std::set<vtkSMProxyManagerEntry>::iterator iter = entriesToRemove.begin();
  while (iter != entriesToRemove.end())
  {
    vtkSMSessionProxyManager::RegisteredProxyInformation info;
    info.Proxy = iter->Proxy;
    info.GroupName = iter->Group.c_str();
    info.ProxyName = iter->Name.c_str();
    info.Type = vtkSMSessionProxyManager::RegisteredProxyInformation::PROXY;

    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
    this->UnMarkProxyAsModified(info.Proxy);

    // Move forward
    iter++;
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxy(vtkSMProxy* proxy)
{
  // Find tuples
  std::set<vtkSMProxyManagerEntry> tuplesToRemove;
  this->Internals->FindProxyTuples(proxy, tuplesToRemove);

  // Remove tuples
  std::set<vtkSMProxyManagerEntry>::iterator iter = tuplesToRemove.begin();
  while (iter != tuplesToRemove.end())
  {
    this->UnRegisterProxy(iter->Group.c_str(), iter->Name.c_str(), iter->Proxy);
    iter++;
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterProxy(
  const char* groupname, const char* name, vtkSMProxy* proxy)
{
  if (!proxy)
  {
    return;
  }

  if (groupname == nullptr)
  {
    vtkErrorMacro("'groupname' cannot be NULL.");
    return;
  }
  if (name == nullptr || name[0] == 0)
  {
    // come up with a new name and register the proxy.
    this->RegisterProxy(groupname, proxy);
    return;
  }

  vtkSMProxyManagerProxyListType& proxy_list = this->Internals->RegisteredProxyMap[groupname][name];
  if (proxy_list.Contains(proxy))
  {
    return;
  }

  // Add Tuple
  this->Internals->RegisteredProxyTuple.insert(vtkSMProxyManagerEntry(groupname, name, proxy));

  vtkSMProxyManagerProxyInfo* proxyInfo = vtkSMProxyManagerProxyInfo::New();
  proxy_list.push_back(proxyInfo);
  proxyInfo->FastDelete();

  proxyInfo->Proxy = proxy;
  // Add observers to note proxy modification.
  proxyInfo->ModifiedObserverTag =
    proxy->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);
  proxyInfo->StateChangedObserverTag =
    proxy->AddObserver(vtkCommand::StateChangedEvent, this->Observer);
  proxyInfo->UpdateObserverTag = proxy->AddObserver(vtkCommand::UpdateEvent, this->Observer);
  proxyInfo->UpdateInformationObserverTag =
    proxy->AddObserver(vtkCommand::UpdateInformationEvent, this->Observer);
  // Note, these observer will be removed in the destructor of proxyInfo.

  // Fire event.
  vtkSMSessionProxyManager::RegisteredProxyInformation info;
  info.Proxy = proxy;
  info.GroupName = groupname;
  info.ProxyName = name;
  info.Type = vtkSMSessionProxyManager::RegisteredProxyInformation::PROXY;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateRegisteredProxies(
  const char* groupname, int modified_only /*=1*/)
{
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.find(groupname);
  if (it != this->Internals->RegisteredProxyMap.end())
  {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
    {
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
      {
        // Check is proxy is in the modified set.
        if (!modified_only ||
          this->Internals->ModifiedProxies.find(it3->GetPointer()->Proxy.GetPointer()) !=
            this->Internals->ModifiedProxies.end())
        {
          it3->GetPointer()->Proxy.GetPointer()->UpdateVTKObjects();
          // FIXME: ASYNC
          // it3->GetPointer()->Proxy.GetPointer()->UpdatePipelineInformation();
        }
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateRegisteredProxies(int modified_only /*=1*/)
{
  vtksys::RegularExpression prototypesRe("_prototypes$");

  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
  {
    if (prototypesRe.find(it->first))
    {
      // skip the prototypes.
      continue;
    }

    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
    for (; it2 != it->second.end(); it2++)
    {
      // Check is proxy is in the modified set.
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
      {
        // Check if proxy is in the modified set.
        if (!modified_only ||
          this->Internals->ModifiedProxies.find(it3->GetPointer()->Proxy.GetPointer()) !=
            this->Internals->ModifiedProxies.end())
        {
          it3->GetPointer()->Proxy.GetPointer()->UpdateVTKObjects();
          // FIXME: ASYNC
          // it3->GetPointer()->Proxy.GetPointer()->UpdatePipelineInformation();
        }
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateRegisteredProxiesInOrder(int modified_only /*=1*/)
{
  this->UpdateInputProxies = 1;
  this->UpdateRegisteredProxies(modified_only);
  this->UpdateInputProxies = 0;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UpdateProxyInOrder(vtkSMProxy* proxy)
{
  this->UpdateInputProxies = 1;
  proxy->UpdateVTKObjects();
  this->UpdateInputProxies = 0;
}

//---------------------------------------------------------------------------
int vtkSMSessionProxyManager::GetNumberOfLinks()
{
  return static_cast<int>(this->Internals->RegisteredLinkMap.size());
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetLinkName(int idx)
{
  int numlinks = this->GetNumberOfLinks();
  if (idx >= numlinks)
  {
    return nullptr;
  }
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.begin();
  for (int i = 0; i < idx; i++)
  {
    it++;
  }
  return it->first.c_str();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterLink(const char* name, vtkSMLink* link)
{
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
  {
    vtkWarningMacro("Replacing previously registered link with name " << name);
  }
  this->Internals->RegisteredLinkMap[name] = link;

  vtkSMSessionProxyManager::RegisteredProxyInformation info;
  info.Proxy = nullptr;
  info.GroupName = nullptr;
  info.ProxyName = name;
  info.Type = vtkSMSessionProxyManager::RegisteredProxyInformation::LINK;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
vtkSMLink* vtkSMSessionProxyManager::GetRegisteredLink(const char* name)
{
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
  {
    return it->second.GetPointer();
  }
  return nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetRegisteredLinkName(vtkSMLink* link)
{
  for (vtkSMSessionProxyManagerInternals::LinkType::iterator it =
         this->Internals->RegisteredLinkMap.begin();
       it != this->Internals->RegisteredLinkMap.end(); it++)
  {
    if (it->second.GetPointer() == link)
    {
      return it->first.c_str();
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterLink(const char* name)
{
  std::string nameHolder = (name ? name : "");
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.find(name);
  if (it != this->Internals->RegisteredLinkMap.end())
  {
    this->Internals->RegisteredLinkMap.erase(it);

    vtkSMSessionProxyManager::RegisteredProxyInformation info;
    info.Proxy = nullptr;
    info.GroupName = nullptr;
    info.ProxyName = nameHolder.c_str();
    info.Type = vtkSMSessionProxyManager::RegisteredProxyInformation::LINK;
    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterAllLinks()
{
  this->Internals->RegisteredLinkMap.clear();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::ExecuteEvent(vtkObject* obj, unsigned long event, void* data)
{
  // Check source object
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(obj);

  // Manage proxy modification call back to mark proxy dirty...
  if (proxy)
  {
    switch (event)
    {
      case vtkCommand::PropertyModifiedEvent:
      {
        // Some property on the proxy has been modified.
        this->MarkProxyAsModified(proxy);
        vtkSMSessionProxyManager::ModifiedPropertyInformation info;
        info.Proxy = proxy;
        info.PropertyName = reinterpret_cast<const char*>(data);
        if (info.PropertyName)
        {
          this->InvokeEvent(vtkCommand::PropertyModifiedEvent, &info);
        }
      }
      break;

      case vtkCommand::StateChangedEvent:
      {
        // I wonder if I need to mark the proxy modified. Cause when state
        // changes, the properties are pushed as well so ideally we should call
        // MarkProxyAsModified() and then UnMarkProxyAsModified() :).
        // this->MarkProxyAsModified(proxy);

        vtkSMSessionProxyManager::StateChangedInformation info;
        info.Proxy = proxy;
        info.StateChangeElement = reinterpret_cast<vtkPVXMLElement*>(data);
        if (info.StateChangeElement)
        {
          this->InvokeEvent(vtkCommand::StateChangedEvent, &info);
        }
      }
      break;

      case vtkCommand::UpdateInformationEvent:
        this->InvokeEvent(vtkCommand::UpdateInformationEvent, proxy);
        break;

      case vtkCommand::UpdateEvent:
        // Proxy has been updated.
        this->UnMarkProxyAsModified(proxy);
        break;
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::MarkProxyAsModified(vtkSMProxy* proxy)
{
  this->Internals->ModifiedProxies.insert(proxy);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnMarkProxyAsModified(vtkSMProxy* proxy)
{
  vtkSMSessionProxyManagerInternals::SetOfProxies::iterator it =
    this->Internals->ModifiedProxies.find(proxy);
  if (it != this->Internals->ModifiedProxies.end())
  {
    this->Internals->ModifiedProxies.erase(it);
  }
}
//---------------------------------------------------------------------------
int vtkSMSessionProxyManager::AreProxiesModified()
{
  return this->Internals->ModifiedProxies.empty() ? 0 : 1;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadXMLState(
  const char* filename, vtkSMStateLoader* loader /*=nullptr*/)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  parser->Parse();

  this->LoadXMLState(parser->GetRootElement(), loader);
  parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadXMLState(vtkPVXMLElement* rootElement,
  vtkSMStateLoader* loader /*=nullptr*/, bool keepOriginalIds /*=false*/)
{
  if (!rootElement)
  {
    return;
  }

  bool prev = this->InLoadXMLState;
  this->InLoadXMLState = true;
  vtkSmartPointer<vtkSMStateLoader> spLoader;
  if (!loader)
  {
    spLoader = vtkSmartPointer<vtkSMStateLoader>::New();
    spLoader->SetSessionProxyManager(this);
  }
  else
  {
    spLoader = loader;
  }
  if (spLoader->LoadState(rootElement, keepOriginalIds))
  {
    vtkSMSessionProxyManager::LoadStateInformation info;
    info.RootElement = rootElement;
    info.ProxyLocator = spLoader->GetProxyLocator();
    this->InvokeEvent(vtkCommand::LoadStateEvent, &info);
  }
  this->InLoadXMLState = prev;
}

//---------------------------------------------------------------------------
bool vtkSMSessionProxyManager::SaveXMLState(const char* filename)
{
  vtkPVXMLElement* rootElement = this->SaveXMLState();
  vtksys::ofstream os(filename, ios::out);
  if (!os.is_open())
  {
    return false;
  }
  rootElement->PrintXML(os, vtkIndent());
  rootElement->Delete();
  return true;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSessionProxyManager::SaveXMLState()
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("GenericParaViewApplication");
  root->AddNestedElement(this->GetXMLState({}));

  vtkSMSessionProxyManager::LoadStateInformation info;
  info.RootElement = root;
  info.ProxyLocator = nullptr;
  this->InvokeEvent(vtkCommand::SaveStateEvent, &info);
  return root;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::CollectReferredProxies(
  vtkSMProxyManagerProxySet& setOfProxies, vtkSMProxy* proxy)
{
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    // FIXME: ASYNC
    // vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(iter->GetProperty());
    // for (unsigned int cc = 0; pp && (pp->GetNumberOfProxies() > cc); cc++)
    // {
    //   vtkSMProxy* referredProxy = pp->GetProxy(cc);
    //   if (referredProxy)
    //   {
    //     setOfProxies.insert(referredProxy);
    //     this->CollectReferredProxies(setOfProxies, referredProxy);
    //   }
    // }
    abort();
  }
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkPVXMLElement> vtkSMSessionProxyManager::GetXMLState(
  const std::set<vtkSMProxy*>& restrictionSet, bool forceRestriction)
{
  vtkSmartPointer<vtkPVXMLElement> rootElement = vtkSmartPointer<vtkPVXMLElement>::New();
  rootElement->SetName("ServerManagerState");

  // Set version number on the state element.
  std::ostringstream version_string;
  version_string << vtkPVCoreApplication::GetVersionMajor() << "."
                 << vtkPVCoreApplication::GetVersionMinor() << "."
                 << vtkPVCoreApplication::GetVersionPatch();
  rootElement->AddAttribute("version", version_string.str().c_str());

  std::set<vtkSMProxy*> visited_proxies; // set of proxies already added.
  std::set<std::pair<std::string, std::string>> visited_proxy_types;

  // First save the state of all proxies
  vtkSMSessionProxyManagerInternals::ProxyGroupType::iterator it =
    this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
  {
    vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();

    const char* colname = it->first.c_str();

    // Do not save the state of prototypes.
    const char* protstr = "_prototypes";
    int do_group = 1;
    if (strlen(colname) > strlen(protstr))
    {
      const char* newstr = colname + strlen(colname) - strlen(protstr);
      if (strcmp(newstr, protstr) == 0)
      {
        do_group = 0;
      }
    }
    else if (colname[0] == '_')
    {
      do_group = 0;
    }
    if (!do_group)
    {
      continue;
    }

    // save the states of all proxies in this group.
    for (; it2 != it->second.end(); it2++)
    {
      vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
      for (; it3 != it2->second.end(); ++it3)
      {
        auto proxy = it3->GetPointer()->Proxy.GetPointer();
#if 0 // FIXME: ASYNC
        if (auto settings = vtkSMSettingsProxy::SafeDownCast(proxy))
        {
          // skip setting proxies that are not serializable.
          if (!settings->GetIsSerializable())
          {
            continue;
          }
        }
#endif
        if (visited_proxies.find(proxy) != visited_proxies.end())
        {
          // proxy has been saved.
          continue;
        }
        if (restrictionSet.empty() && forceRestriction)
        {
          // caller is explictly forcing restriction of the state.
          continue;
        }
        if (!restrictionSet.empty() && restrictionSet.find(proxy) == restrictionSet.end())
        {
          // we're skipping this proxy on request.
          continue;
        }

        proxy->SaveXMLState(rootElement);
        visited_proxies.insert(proxy);
        visited_proxy_types.insert(std::make_pair(proxy->GetXMLGroup(), proxy->GetXMLName()));
      }
    }
  }

  // Save the proxy collections. This is done separately because
  // one proxy can be in more than one group.
  it = this->Internals->RegisteredProxyMap.begin();
  for (; it != this->Internals->RegisteredProxyMap.end(); it++)
  {
    // Do not save the state of prototypes.
    const char* protstr = "_prototypes";
    int do_group = 1;
    if (strlen(it->first.c_str()) > strlen(protstr))
    {
      const char* newstr = it->first.c_str() + strlen(it->first.c_str()) - strlen(protstr);
      if (strcmp(newstr, protstr) == 0)
      {
        do_group = 0;
      }
    }
    if (do_group)
    {
      vtkPVXMLElement* collectionElement = vtkPVXMLElement::New();
      collectionElement->SetName("ProxyCollection");
      collectionElement->AddAttribute("name", it->first.c_str());
      vtkSMProxyManagerProxyMapType::iterator it2 = it->second.begin();
      bool some_proxy_added = false;
      for (; it2 != it->second.end(); it2++)
      {
        vtkSMProxyManagerProxyListType::iterator it3 = it2->second.begin();
        for (; it3 != it2->second.end(); ++it3)
        {
          auto curproxy = it3->GetPointer()->Proxy.GetPointer();
          if (visited_proxies.find(curproxy) != visited_proxies.end())
          {
            vtkPVXMLElement* itemElement = vtkPVXMLElement::New();
            itemElement->SetName("Item");
            itemElement->AddAttribute("id", curproxy->GetGlobalID());
            itemElement->AddAttribute("name", it2->first.c_str());
            if (curproxy->GetLogName() != nullptr)
            {
              itemElement->AddAttribute("logname", curproxy->GetLogName());
            }
            collectionElement->AddNestedElement(itemElement);
            itemElement->Delete();
            some_proxy_added = true;
          }
        }
      }
      // Avoid addition of empty groups.
      if (some_proxy_added)
      {
        rootElement->AddNestedElement(collectionElement);
      }
      collectionElement->Delete();
    }
  }

  vtkPVXMLElement* defs = vtkPVXMLElement::New();
  defs->SetName("CustomProxyDefinitions");
  this->SaveCustomProxyDefinitions(defs);
  // remove definitions from defs that are not relevant for the proxies in the
  // state file.
  std::vector<vtkPVXMLElement*> to_remove;
  for (unsigned int cc = 0, max = defs->GetNumberOfNestedElements(); cc < max; ++cc)
  {
    auto child = defs->GetNestedElement(cc);
    auto cgroup = child->GetAttributeOrEmpty("group");
    auto cname = child->GetAttributeOrEmpty("name");
    if (child->GetName() && strcmp(child->GetName(), "CustomProxyDefinition") == 0 &&
      visited_proxy_types.find(std::make_pair(cgroup, cname)) == visited_proxy_types.end())
    {
      to_remove.push_back(child);
    }
  }
  for (auto child : to_remove)
  {
    defs->RemoveNestedElement(child);
  }
  to_remove.clear();
  rootElement->AddNestedElement(defs);
  defs->Delete();

  // Save links
  vtkPVXMLElement* links = vtkPVXMLElement::New();
  links->SetName("Links");
  this->SaveRegisteredLinks(links);
  rootElement->AddNestedElement(links);
  links->Delete();

  // Save information about links with `settings` proxies.
  auto settingsIter = this->Internals->RegisteredProxyMap.find("settings");
  if (settingsIter != this->Internals->RegisteredProxyMap.end())
  {
    vtkNew<vtkPVXMLElement> settings;
    settings->SetName("Settings");
    rootElement->AddNestedElement(settings);
    for (const auto& pair : settingsIter->second)
    {
      for (auto& pxminfo : pair.second)
      {
#if 0 // FIXME: ASYNC
        if (auto settingsProxy = vtkSMSettingsProxy::SafeDownCast(pxminfo->Proxy))
        {
          settingsProxy->SaveLinksState(settings);
        }
#endif
      }
    }
  }
  return rootElement;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterCustomProxyDefinitions()
{
  // assert(this->ProxyDefinitionManager != 0);
  // this->ProxyDefinitionManager->ClearCustomProxyDefinitions();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterCustomProxyDefinition(const char* group, const char* name)
{
  // assert(this->ProxyDefinitionManager != 0);
  // this->ProxyDefinitionManager->RemoveCustomProxyDefinition(group, name);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterCustomProxyDefinition(
  const char* group, const char* name, vtkPVXMLElement* top)
{
  // assert(this->ProxyDefinitionManager != 0);
  // this->ProxyDefinitionManager->AddCustomProxyDefinition(group, name, top);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadCustomProxyDefinitions(vtkPVXMLElement* root)
{
  // assert(this->ProxyDefinitionManager != 0);
  // this->ProxyDefinitionManager->LoadCustomProxyDefinitions(root);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::LoadCustomProxyDefinitions(const char* filename)
{
  // assert(this->ProxyDefinitionManager != 0);
  // vtkPVXMLParser* parser = vtkPVXMLParser::New();
  // parser->SetFileName(filename);
  // if (!parser->Parse())
  // {
  //   vtkErrorMacro("Failed to parse file : " << filename);
  //   return;
  // }
  // this->ProxyDefinitionManager->LoadCustomProxyDefinitions(parser->GetRootElement());
  // parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::SaveCustomProxyDefinitions(vtkPVXMLElement* rootElement)
{
  // assert("Definition Manager should exist" && this->ProxyDefinitionManager != 0);
  // this->ProxyDefinitionManager->SaveCustomProxyDefinitions(rootElement);
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::SaveRegisteredLinks(vtkPVXMLElement* rootElement)
{
  vtkSMSessionProxyManagerInternals::LinkType::iterator it =
    this->Internals->RegisteredLinkMap.begin();
  for (; it != this->Internals->RegisteredLinkMap.end(); ++it)
  {
    it->second.GetPointer()->SaveXMLState(it->first.c_str(), rootElement);
  }
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSessionProxyManager::GetProxyHints(
  const char* groupName, const char* proxyName)
{
  if (!groupName || !proxyName)
  {
    return nullptr;
  }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  return proxy ? proxy->GetHints() : nullptr;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSessionProxyManager::GetPropertyHints(
  const char* groupName, const char* proxyName, const char* propertyName)
{
  if (!groupName || !proxyName || !propertyName)
  {
    return nullptr;
  }

  vtkSMProxy* proxy = this->GetPrototypeProxy(groupName, proxyName);
  if (proxy)
  {
    vtkSMProperty* prop = proxy->GetProperty(propertyName);
    if (prop)
    {
      return prop->GetHints();
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
bool vtkSMSessionProxyManager::LoadConfigurationXML(const char* xml)
{
  auto* pdm = this->GetProxyDefinitionManager();
  return pdm ? pdm->LoadConfigurationXML(xml) : false;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UpdateInputProxies: " << this->UpdateInputProxies << endl;
}

//---------------------------------------------------------------------------
bool vtkSMSessionProxyManager::HasDefinition(const char* groupName, const char* proxyName)
{
  auto* pdm = this->GetProxyDefinitionManager();
  return pdm && pdm->FindProxy(groupName, proxyName) != nullptr;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterSelectionModel(
  const char* name, vtkSMProxySelectionModel* model)
{
  if (!model)
  {
    vtkErrorMacro("Cannot register a null model.");
    return;
  }
  if (!name)
  {
    vtkErrorMacro("Cannot register model with no name.");
    return;
  }

  vtkSMProxySelectionModel* curmodel = this->GetSelectionModel(name);
  if (curmodel && curmodel == model)
  {
    // already registered.
    return;
  }

  if (curmodel)
  {
    vtkWarningMacro("Replacing existing selection model: " << name);
  }
  // model->SetSession(this->GetSession());
  this->Internals->SelectionModels[name] = model;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterSelectionModel(const char* name)
{
  this->Internals->SelectionModels.erase(name);
}

//---------------------------------------------------------------------------
vtkSMProxySelectionModel* vtkSMSessionProxyManager::GetSelectionModel(const char* name)
{
  vtkSMSessionProxyManagerInternals::SelectionModelsType::iterator iter =
    this->Internals->SelectionModels.find(name);
  if (iter == this->Internals->SelectionModels.end())
  {
    return nullptr;
  }

  return iter->second;
}
//---------------------------------------------------------------------------
vtkIdType vtkSMSessionProxyManager::GetNumberOfSelectionModel()
{
  return static_cast<vtkIdType>(this->Internals->SelectionModels.size());
}

//---------------------------------------------------------------------------
vtkSMProxySelectionModel* vtkSMSessionProxyManager::GetSelectionModelAt(int idx)
{
  vtkSMSessionProxyManagerInternals::SelectionModelsType::iterator iter =
    this->Internals->SelectionModels.begin();
  for (int i = 0; i < idx; i++)
  {
    if (iter == this->Internals->SelectionModels.end())
    {
      // Out of range
      return nullptr;
    }
    iter++;
  }

  return iter->second;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::FindProxy(
  const char* reggroup, const char* xmlgroup, const char* xmltype)
{
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(this);
  iter->SetModeToOneGroup();

  for (iter->Begin(reggroup); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProxy* proxy = iter->GetProxy();
    if (proxy != nullptr && proxy->GetXMLGroup() && proxy->GetXMLName() &&
      strcmp(proxy->GetXMLGroup(), xmlgroup) == 0 && strcmp(proxy->GetXMLName(), xmltype) == 0)
    {
      return proxy;
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkClientSession* vtkSMSessionProxyManager::GetSession() const
{
  return this->Session.GetPointer();
}

//----------------------------------------------------------------------------
vtkProxyDefinitionManager* vtkSMSessionProxyManager::GetProxyDefinitionManager() const
{
  return this->Session ? this->Session->GetProxyDefinitionManager() : nullptr;
}
