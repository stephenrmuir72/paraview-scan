/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyManager.h"
#include "vtkSMProxyManagerInternals.h"

#include "vtkChannelSubscription.h"
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProxyDefinitionManager.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkSMInstantiator.h"
#include "vtkSMProxy.h"

#include <sstream>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

vtkStandardNewMacro(vtkSMProxyManager);
//----------------------------------------------------------------------------
vtkSMProxyManager::vtkSMProxyManager()
{
}

//----------------------------------------------------------------------------
vtkSMProxyManager::~vtkSMProxyManager() = default;

//----------------------------------------------------------------------------
void vtkSMProxyManager::SetSession(vtkClientSession* session)
{
  this->Internals.reset(new vtkInternals(session, this));
}

//----------------------------------------------------------------------------
vtkClientSession* vtkSMProxyManager::GetSession() const
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return internals.Session;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSMProxy> vtkSMProxyManager::NewProxy(
  const std::string& group, const std::string& name)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  if (group.empty() || name.empty())
  {
    vtkLogF(ERROR, "invalid group or name; cannot be empty!");
    return nullptr;
  }

  auto* pdm = internals.Session->GetProxyDefinitionManager();
  auto xml = pdm->FindProxy(group, name);
  if (!xml)
  {
    vtkLogF(ERROR, "Unknown proxy type (%s, %s)", group.c_str(), name.c_str());
    return nullptr;
  }

  // TODO: eventually, vtkProxyDefinitionManager will directly provide us with
  // pugi::xml_node rather than vtkPVXMLElement.
  std::ostringstream stream;
  xml->PrintXML(stream, vtkIndent());
  pugi::xml_document doc;
  auto result = doc.load_string(stream.str().c_str());
  if (!result)
  {
    vtkLogF(ERROR, "unexpected parsing error!");
    abort();
  }

  auto node = doc.document_element();

  const auto className = fmt::format("vtkSM{}", node.name());
  auto object = vtkSMInstantiator::CreateInstance(className);
  if (!object)
  {
    vtkLogF(ERROR, "Unknown class type (%s)", className.c_str());
    return nullptr;
  }

  auto* proxy = vtkSMProxy::SafeDownCast(object);
  if (!proxy)
  {
    vtkLogF(ERROR, "'%s' must be a 'vtkSMProxy' subclass.", className.c_str());
    return nullptr;
  }

  proxy->SetGroup(group);
  proxy->SetName(name);
  proxy->SetSession(this->GetSession());
  proxy->SetGlobalID(internals.NextGlobalID++);
  internals.Proxies[proxy->GetGlobalID()] = proxy;
  if (proxy->ReadXMLAttributes(node))
  {
    // For every proxy we create, add observers. This lets the vtkSMProxyManager
    // act as a central place to receive all notifications from all proxies.
    // Previously, this was only done for registered proxies. I don't see any
    // disadvantage to doing that for all created proxies.
    proxy->AddObserver(
      vtkCommand::PropertyModifiedEvent, this, &vtkSMProxyManager::HandleProxyEvent);
    proxy->AddObserver(vtkCommand::StateChangedEvent, this, &vtkSMProxyManager::HandleProxyEvent);
    proxy->AddObserver(vtkCommand::UpdateEvent, this, &vtkSMProxyManager::HandleProxyEvent);
    proxy->AddObserver(
      vtkCommand::UpdateInformationEvent, this, &vtkSMProxyManager::HandleProxyEvent);
    return proxy;
  }

  vtkLogF(ERROR, "Failed to create proxy (%s, %s)", group.c_str(), name.c_str());
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::RegisterProxy(
  const std::string& group, const std::string& name, const vtkSmartPointer<vtkSMProxy>& proxy)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  if (group.empty() || name.empty())
  {
    vtkLogF(ERROR, "Invalid group or name");
    return;
  }

  if (proxy == nullptr)
  {
    vtkLogF(WARNING, "Cannot register null proxy");
    return;
  }

  const auto key = vtkInternals::GetKey(group, name);
  auto range = internals.RegisteredProxies.equal_range(key);
  if (std::find_if(range.first, range.second,
        [&](const auto& pair) { return pair.second == proxy; }) != range.second)
  {
    // proxy already registered with the same name; don't re-register it.
    vtkLogF(WARNING, "Cannot reregister proxy under same name");
    return;
  }

  internals.RegisteredProxies.emplace(key, proxy);
  /// TODO: fire events
}

//----------------------------------------------------------------------------
std::string vtkSMProxyManager::RegisterProxy(
  const std::string& group, const vtkSmartPointer<vtkSMProxy>& proxy)
{
  if (group.empty() || proxy == nullptr)
  {
    vtkLogF(ERROR, "empty arguments.");
    return {};
  }

  // FIXME:
  // const auto label = vtkSMCoreUtilities::SanitizeName(proxy->GetXMLLabel());
  const auto label = proxy->GetXMLLabel();
  const auto name = this->GetUniqueProxyName(group, label);
  this->RegisterProxy(group, name, proxy);
  return name;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxy(
  const std::string& group, const std::string& name, const vtkSmartPointer<vtkSMProxy>& proxy)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  const auto key = vtkInternals::GetKey(group, name);
  const auto range = internals.RegisteredProxies.equal_range(key);
  auto iter =
    std::find_if(range.first, range.second, [&](const auto& pair) { return pair.second == proxy; });
  if (iter != range.second)
  {
    internals.RegisteredProxies.erase(iter);
    // TODO: fire events
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxies(const std::string& group, const std::string& name)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  const auto key = vtkInternals::GetKey(group, name);
  const auto range = internals.RegisteredProxies.equal_range(key);
  for (auto iter = range.first; iter != range.second;)
  {
    iter = internals.RegisteredProxies.erase(iter);
    // TODO: fire events
  }
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::UnRegisterProxy(const vtkSmartPointer<vtkSMProxy>& proxy)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  for (auto iter = internals.RegisteredProxies.begin(); iter != internals.RegisteredProxies.end();)
  {
    if (iter->second == proxy)
    {
      iter = internals.RegisteredProxies.erase(iter);
      // TODO: fire events.
    }
    else
    {
      ++iter;
    }
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSMProxy> vtkSMProxyManager::FindProxy(vtkTypeUInt32 gid) const
{
  const auto& internals = (*this->Internals);
  auto iter = internals.Proxies.find(gid);
  return iter != internals.Proxies.end() ? iter->second.GetPointer() : nullptr;
}

//----------------------------------------------------------------------------
std::string vtkSMProxyManager::GetUniqueProxyName(
  const std::string& group, const std::string& prefix, bool alwaysAppend) const
{
  if (group.empty() || prefix.empty())
  {
    return {};
  }

  std::string name = prefix;
  int next = 1;
  if (alwaysAppend)
  {
    name += std::to_string(next++);
  }

  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  auto iter = internals.RegisteredProxies.find(vtkInternals::GetKey(group, name));
  while (iter != internals.RegisteredProxies.end())
  {
    if (next == VTK_INT_MAX)
    {
      vtkLogF(ERROR, "Failed to come up with unique name!");
      abort();
    }

    name = prefix + std::to_string(next++);
    iter = internals.RegisteredProxies.find(vtkInternals::GetKey(group, name));
  }
  return name;
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::HandleProxyEvent(vtkObject* sender, unsigned long eventId, void* callData)
{
  // TODO: ASYNC
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::MarkPipelineUpdated(vtkSMProxy* proxy)
{
  proxy->MarkPipelineUpdated();
}

//----------------------------------------------------------------------------
void vtkSMProxyManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
