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
#include "vtkSMSessionProxyManagerInternals.h"

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
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSourceProxy.h"
// #include "vtkSMSettingsProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkSMOutputPort.h"
#include "vtkSMStateLoader.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"
#include "vtkStringUtilities.h"
#include "vtkVersion.h"

#include "vtksys/FStream.hxx"
#include "vtksys/RegularExpression.hxx"

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <vector>

//*****************************************************************************
vtkStandardNewMacro(vtkSMSessionProxyManager);
//---------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMSessionProxyManager::New(vtkClientSession* session)
{
  assert(session != nullptr);
  auto* pxm = vtkSMSessionProxyManager::New();
  pxm->Internals->SetSession(session);
  return pxm;
}

//---------------------------------------------------------------------------
vtkSMSessionProxyManager::vtkSMSessionProxyManager()
  : Internals(new vtkSMSessionProxyManagerInternals(this))
  , InLoadXMLState(false)
{
}

//---------------------------------------------------------------------------
vtkSMSessionProxyManager::~vtkSMSessionProxyManager() = default;

//----------------------------------------------------------------------------
void vtkSMSessionProxyManager::InstantiateGroupPrototypes(const char* groupName)
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  auto* pdm = this->GetProxyDefinitionManager();
  assert(pdm != nullptr);

  // Find the XML elements from which the proxies can be instantiated and
  // initialized
  for (const auto& item : pdm->GetDefinitions(groupName ? groupName : ""))
  {
    const auto group = item.GetGroup();
    const auto name = item.GetName();
    const auto regGroup = group + "_prototypes";
    if (this->GetProxy(regGroup.c_str(), name.c_str()) == nullptr)
    {
      if (vtkSMProxy* proxy = this->NewProxy(group.c_str(), name.c_str()))
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
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  const std::string suffix("_prototypes");

  auto iter = internals.RegisteredProxies.begin();
  while (iter != internals.RegisteredProxies.end())
  {
    const auto& key = iter->first;
    const auto& group = vtkSMSessionProxyManagerInternals::GetGroup(key);
    if (vtkStringUtilities::EndsWith(group, suffix))
    {
      iter = internals.RegisteredProxies.erase(iter);
    }
    else
    {
      ++iter;
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
  auto& internals = (*this->Internals);
  std::ostringstream cname;
  cname << "vtkSM" << pelement->GetName();
  auto proxy = vtkReflection::CreateInstance<vtkSMProxy>(cname.str());
  if (proxy)
  {
    // XMLName/XMLGroup should be set before ReadXMLAttributes so sub proxy
    // can be found based on their names when sent to the PM Side
    proxy->ProxyManager = this;
    proxy->GlobalID = internals.NextGlobalID++;
    proxy->SetXMLGroup(groupname);
    proxy->SetXMLName(proxyname);
    proxy->ReadXMLAttributes(this, pelement);
    proxy->Register(this);
    internals.Proxies[proxy->GlobalID] =
      proxy; // FIXME: if this a prototype, this need  to be fixed.

    // Add observers to note proxy modification.
    proxy->AddObserver(vtkCommand::PropertyModifiedEvent, internals.Observer);
    proxy->AddObserver(vtkCommand::StateChangedEvent, internals.Observer);
    proxy->AddObserver(vtkCommand::UpdateInformationEvent, internals.Observer);

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
unsigned int vtkSMSessionProxyManager::GetNumberOfProxies(const char* group) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  return std::accumulate(internals.RegisteredProxies.begin(), internals.RegisteredProxies.end(),
    static_cast<unsigned int>(0), [group](unsigned int sum, const auto& pair) {
      return sum + (vtkSMSessionProxyManagerInternals::GetGroup(pair.first) == group ? 1 : 0);
    });
}

//---------------------------------------------------------------------------
// No errors are raised if a proxy definition for the requested proxy is not
// found.
vtkSMProxy* vtkSMSessionProxyManager::GetPrototypeProxy(const char* groupname, const char* name)
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

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
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

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
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  auto iter =
    internals.RegisteredProxies.find(vtkSMSessionProxyManagerInternals::GetKey(group, name));
  return iter != internals.RegisteredProxies.end() ? iter->second.GetPointer() : nullptr;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMSessionProxyManager::GetProxy(const char* name)
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  auto iter = std::find_if(internals.RegisteredProxies.begin(), internals.RegisteredProxies.end(),
    [&name](
      const auto& pair) { return vtkSMSessionProxyManagerInternals::GetName(pair.first) == name; });
  return iter != internals.RegisteredProxies.end() ? iter->second.GetPointer() : nullptr;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::GetProxies(
  const char* group, const char* name, vtkCollection* collection)
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  collection->RemoveAllItems();

  const auto range =
    internals.RegisteredProxies.equal_range(vtkSMSessionProxyManagerInternals::GetKey(group, name));
  for (auto iter = range.first; iter != range.second; ++iter)
  {
    collection->AddItem(iter->second);
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

  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  for (const auto& pair : internals.RegisteredProxies)
  {
    if (vtkSMSessionProxyManagerInternals::GetGroup(pair.first) == groupname &&
      pair.second == proxy)
    {
      names->AddString(vtkSMSessionProxyManagerInternals::GetName(pair.first).c_str());
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

  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  auto key = vtkSMSessionProxyManagerInternals::GetKey(
    groupname, alwaysAppend ? prefix + std::to_string(1) : std::string(prefix));
  if (internals.RegisteredProxies.find(key) == internals.RegisteredProxies.end())
  {
    return vtkSMSessionProxyManagerInternals::GetName(key);
  }

  std::set<std::string> existingNames;

  for (const auto& pair : internals.RegisteredProxies)
  {
    const auto& key = pair.first;
    if (vtkSMSessionProxyManagerInternals::GetGroup(key) == groupname)
    {
      existingNames.insert(vtkSMSessionProxyManagerInternals::GetName(key));
    }
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

  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  for (const auto& pair : internals.RegisteredProxies)
  {
    if (vtkSMSessionProxyManagerInternals::GetGroup(pair.first) == groupname &&
      pair.second == proxy)
    {
      return vtkSMSessionProxyManagerInternals::GetName(pair.first).c_str();
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

  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  unsigned int counter = 0;
  for (const auto& pair : internals.RegisteredProxies)
  {
    if (vtkSMSessionProxyManagerInternals::GetGroup(pair.first) == groupname)
    {
      if (counter == idx)
      {
        return vtkSMSessionProxyManagerInternals::GetName(pair.first).c_str();
      }
      ++counter;
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

  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  unsigned int counter = 0;
  for (const auto& pair : internals.RegisteredProxies)
  {
    if (vtkSMSessionProxyManagerInternals::GetGroup(pair.first) == groupname &&
      pair.second == proxy)
    {
      return vtkSMSessionProxyManagerInternals::GetName(pair.first).c_str();
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
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxy(
  const char* group, const char* name, vtkSMProxy* proxy)
{
  if (!group || !name)
  {
    return;
  }

  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  // Just in case a proxy ref is NOT held outside the ProxyManager iteself
  // Keep one during the full method call so the event could still have a valid
  // proxy object.
  vtkSmartPointer<vtkSMProxy> proxyHolder = proxy;
  const auto key = vtkSMSessionProxyManagerInternals::GetKey(group, name);
  auto iter = std::find_if(internals.RegisteredProxies.begin(), internals.RegisteredProxies.end(),
    [&](const auto& pair) { return pair.first == key && pair.second == proxy; });

  if (iter != internals.RegisteredProxies.end())
  {
    internals.RegisteredProxies.erase(iter);

    vtkSMSessionProxyManager::RegisteredProxyInformation info;
    info.Proxy = proxy;
    info.GroupName = group;
    info.ProxyName = name;
    info.Type = vtkSMSessionProxyManager::RegisteredProxyInformation::PROXY;
    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterProxy(vtkSMProxy* proxy)
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  vtkSmartPointer<vtkSMProxy> holder(proxy);

  std::set<std::pair<std::string, std::string>> keys;
  for (const auto& pair : internals.RegisteredProxies)
  {
    if (pair.second == proxy)
    {
      keys.emplace(pair.first);
    }
  }

  for (const auto& pair : keys)
  {
    this->UnRegisterProxy(pair.first.c_str(), pair.second.c_str(), proxy);
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

  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  const auto range = internals.RegisteredProxies.equal_range(
    vtkSMSessionProxyManagerInternals::GetKey(groupname, name));
  for (auto iter = range.first; iter != range.second; ++iter)
  {
    if (iter->second == proxy)
    {
      // already registered.
      return;
    }
  }

  // Add Tuple
  internals.RegisteredProxies.emplace(
    vtkSMSessionProxyManagerInternals::GetKey(groupname, name), proxy);

  // Fire event.
  vtkSMSessionProxyManager::RegisteredProxyInformation info;
  info.Proxy = proxy;
  info.GroupName = groupname;
  info.ProxyName = name;
  info.Type = vtkSMSessionProxyManager::RegisteredProxyInformation::PROXY;
  this->InvokeEvent(vtkCommand::RegisterEvent, &info);
}

//---------------------------------------------------------------------------
int vtkSMSessionProxyManager::GetNumberOfLinks()
{
  return static_cast<int>(this->Internals->RegisteredLinks.size());
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetLinkName(int idx)
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  if (0 <= idx && static_cast<size_t>(idx) < internals.RegisteredLinks.size())
  {
    return std::next(internals.RegisteredLinks.begin(), idx)->first.c_str();
  }
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::RegisterLink(const char* name, vtkSMLink* link)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  if (internals.RegisteredLinks.find(name) != internals.RegisteredLinks.end())
  {
    vtkWarningMacro("Replacing previously registered link with name " << name);
  }

  internals.RegisteredLinks[name] = link;

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
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  auto iter = internals.RegisteredLinks.find(name);
  return iter != internals.RegisteredLinks.end() ? iter->second.GetPointer() : nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMSessionProxyManager::GetRegisteredLinkName(vtkSMLink* link)
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  for (const auto& pair : internals.RegisteredLinks)
  {
    if (pair.second.GetPointer() == link)
    {
      return pair.first.c_str();
    }
  }

  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMSessionProxyManager::UnRegisterLink(const char* name)
{
  std::string nameHolder = (name ? name : "");
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  auto iter = internals.RegisteredLinks.find(name);
  if (iter != internals.RegisteredLinks.end())
  {
    internals.RegisteredLinks.erase(iter);

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
  this->Internals->RegisteredLinks.clear();
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
        break;
    }
  }
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
vtkSmartPointer<vtkPVXMLElement> vtkSMSessionProxyManager::GetXMLState(
  const std::set<vtkSMProxy*>& restrictionSet, bool forceRestriction)
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

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
  std::map<std::string, std::vector<std::pair<std::string, vtkSMProxy*>>> collections;

  // First save the state of all proxies
  for (const auto& pair : internals.RegisteredProxies)
  {
    const auto& group = vtkSMSessionProxyManagerInternals::GetGroup(pair.first);
    const auto& name = vtkSMSessionProxyManagerInternals::GetName(pair.first);
    if (vtkStringUtilities::EndsWith(group, "_prototypes") || (!group.empty() && group[0] == '_'))
    {
      // skip prototype group
      continue;
    }

    auto& proxy = pair.second;
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
    collections[group].emplace_back(name, proxy);
  }

  // Save the proxy collections. This is done separately because
  // one proxy can be in more than one group.
  for (const auto& pair : collections)
  {
    vtkPVXMLElement* collectionElement = vtkPVXMLElement::New();
    collectionElement->SetName("ProxyCollection");
    collectionElement->AddAttribute("name", pair.first.c_str());
    for (auto& proxyPair : pair.second)
    {
      const auto& name = proxyPair.first;
      auto* curproxy = proxyPair.second;

      vtkPVXMLElement* itemElement = vtkPVXMLElement::New();
      itemElement->SetName("Item");
      itemElement->AddAttribute("id", curproxy->GetGlobalID());
      itemElement->AddAttribute("name", name.c_str());
      if (curproxy->GetLogName() != nullptr)
      {
        itemElement->AddAttribute("logname", curproxy->GetLogName());
      }
      collectionElement->AddNestedElement(itemElement);
      itemElement->Delete();
    }
    rootElement->AddNestedElement(collectionElement);
    collectionElement->Delete();
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
#if 0 // FIXME: ASYNC
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
        if (auto settingsProxy = vtkSMSettingsProxy::SafeDownCast(pxminfo->Proxy))
        {
          settingsProxy->SaveLinksState(settings);
        }
      }
    }
  }
#endif
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
  for (auto& pair : this->Internals->RegisteredLinks)
  {
    pair.second->SaveXMLState(pair.first.c_str(), rootElement);
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
vtkSMProxy* vtkSMSessionProxyManager::FindProxy(vtkTypeUInt32 gid) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  auto iter = internals.Proxies.find(gid);
  if (iter != internals.Proxies.end() && iter->second && iter->second->GetGlobalID() == gid)
  {
    return iter->second;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkClientSession* vtkSMSessionProxyManager::GetSession() const
{
  return this->Internals->Session.GetPointer();
}

//----------------------------------------------------------------------------
vtkProxyDefinitionManager* vtkSMSessionProxyManager::GetProxyDefinitionManager() const
{
  return this->Internals->Session ? this->Internals->Session->GetProxyDefinitionManager() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMSessionProxyManager::ProcessPiggybackedMessages(const vtkNJson& json)
{
  const bool hasDataInformation = json.count("pb_data_informations") > 0;
  if (!hasDataInformation)
  {
    return;
  }

  for (const auto& el : json["pb_data_informations"].items())
  {
    const auto gid = static_cast<vtkTypeUInt32>(std::stoi(el.key()));
    if (auto* proxy = vtkSMSourceProxy::SafeDownCast(this->FindProxy(gid)))
    {
      for (unsigned int idx = 0; idx < proxy->GetNumberOfOutputPorts(); ++idx)
      {
        auto* port = proxy->GetOutputPort(idx);
        auto* dinfo = port->GetDataInformation();
        dinfo->Initialize();
        vtkRemoteObjectProvider::ParseResponse(el.value().at(idx), dinfo);
        dinfo->SetIsObsolete(false);
      }
      proxy->PipelineUpdated();
    }
  }
}
