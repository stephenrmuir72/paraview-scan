// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMSceneGraphProxy.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSceneGraphNodeProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

//----------------------------------------------------------------------------
// The `Children` property is the tree. These helpers read and write it as a
// plain vector so the tree operations below can be expressed as list edits
// (e.g. splice, insert at index) which are not otherwise provided by the
// property API.
vtkSMProxyProperty* ChildrenProperty(vtkSMSceneGraphNodeProxy* node)
{
  return node ? vtkSMProxyProperty::SafeDownCast(node->GetProperty("Children")) : nullptr;
}

// ----------------------------------------------------------------------------
std::vector<vtkSMSceneGraphNodeProxy*> GetChildVector(vtkSMSceneGraphNodeProxy* node)
{
  std::vector<vtkSMSceneGraphNodeProxy*> children;
  vtkSMProxyProperty* prop = ChildrenProperty(node);
  if (!prop)
  {
    return children;
  }
  const unsigned int count = prop->GetNumberOfProxies();
  children.reserve(count);
  for (unsigned int cc = 0; cc < count; ++cc)
  {
    if (auto* child = vtkSMSceneGraphNodeProxy::SafeDownCast(prop->GetProxy(cc)))
    {
      children.push_back(child);
    }
  }
  return children;
}

// ----------------------------------------------------------------------------
void SetChildVector(
  vtkSMSceneGraphNodeProxy* node, const std::vector<vtkSMSceneGraphNodeProxy*>& children)
{
  vtkSMProxyProperty* prop = ChildrenProperty(node);
  if (!prop)
  {
    return;
  }

  std::vector<vtkSMProxy*> raw(children.begin(), children.end());
  if (raw.empty())
  {
    prop->SetNumberOfProxies(0);
  }
  else
  {
    prop->SetProxies(static_cast<unsigned int>(raw.size()), raw.data());
  }
  node->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
// A source binding on a scenegraph node is one output port of one source. The
// "Sources" property is an InputProperty which stores the source and port
// together.
using Binding = std::pair<vtkSMSourceProxy*, int>;

// ----------------------------------------------------------------------------
std::vector<Binding> GetBindings(vtkSMSceneGraphNodeProxy* node)
{
  std::vector<Binding> bindings;
  auto* sources = node ? vtkSMInputProperty::SafeDownCast(node->GetProperty("Sources")) : nullptr;
  if (!sources)
  {
    return bindings;
  }

  const unsigned int count = sources->GetNumberOfProxies();
  bindings.reserve(count);
  for (unsigned int cc = 0; cc < count; ++cc)
  {
    auto* source = vtkSMSourceProxy::SafeDownCast(sources->GetProxy(cc));
    if (!source)
    {
      continue;
    }
    bindings.emplace_back(source, static_cast<int>(sources->GetOutputPortForConnection(cc)));
  }
  return bindings;
}

// ----------------------------------------------------------------------------
void SetBindings(vtkSMSceneGraphNodeProxy* node, const std::vector<Binding>& bindings)
{
  auto* sources = node ? vtkSMInputProperty::SafeDownCast(node->GetProperty("Sources")) : nullptr;
  if (!sources)
  {
    return;
  }

  std::vector<vtkSMProxy*> proxies;
  std::vector<unsigned int> ports;
  proxies.reserve(bindings.size());
  ports.reserve(bindings.size());
  for (const auto& binding : bindings)
  {
    proxies.push_back(binding.first);
    ports.push_back(static_cast<unsigned int>(binding.second));
  }

  if (proxies.empty())
  {
    sources->SetNumberOfProxies(0);
  }
  else
  {
    sources->SetProxies(static_cast<unsigned int>(proxies.size()), proxies.data(), ports.data());
  }
  node->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
const char* NameOf(vtkSMSceneGraphNodeProxy* node);

// ----------------------------------------------------------------------------
// Store details encountered during a walk of the tree
struct Walk
{
  // the visitor returned false, which is how a search reports success
  bool Stopped = false;

  // the first node that was encountered twice, if any
  vtkSMSceneGraphNodeProxy* Repeated = nullptr;
};

// ----------------------------------------------------------------------------
std::string NotATreeMessage(vtkSMSceneGraphNodeProxy* repeated)
{
  const char* name = NameOf(repeated);
  return std::string("Node '") + (name ? name : "?") +
    "' is reachable more than one way, the graph is not a tree.";
}

// ----------------------------------------------------------------------------
// Visit the start node and everything below it, once each, calling the visit
// method on every node. Visitors can return false to stop the walk.
//
// Here we check whether we've already seen each node before we visit it, as
// if we have, that would indicate a diamond or a cycle, neither of which is
// a valid tree. Our own methods won't build such a graph but a direct set of
// a Children property could.
template <typename Visitor>
Walk VisitSubtree(vtkSMSceneGraphNodeProxy* start, Visitor visit)
{
  Walk walk;
  std::vector<vtkSMSceneGraphNodeProxy*> pending;
  std::set<vtkSMSceneGraphNodeProxy*> seen;
  if (start)
  {
    pending.push_back(start);
  }

  while (!pending.empty())
  {
    vtkSMSceneGraphNodeProxy* current = pending.back();
    pending.pop_back();
    if (!current)
    {
      continue;
    }
    if (!seen.insert(current).second)
    {
      if (!walk.Repeated)
      {
        walk.Repeated = current;
      }
      continue;
    }
    if (!visit(current))
    {
      walk.Stopped = true;
      return walk;
    }
    for (auto* child : GetChildVector(current))
    {
      pending.push_back(child);
    }
  }
  return walk;
}

// ----------------------------------------------------------------------------
// Return the node below the root that lists the given node as a child, or null.
vtkSMSceneGraphNodeProxy* FindParentOf(
  vtkSMSceneGraphNodeProxy* root, vtkSMSceneGraphNodeProxy* node, Walk& walk)
{
  vtkSMSceneGraphNodeProxy* parent = nullptr;
  walk = VisitSubtree(root,
    [&](vtkSMSceneGraphNodeProxy* current)
    {
      for (auto* child : GetChildVector(current))
      {
        if (child == node)
        {
          parent = current;
          return false;
        }
      }
      return true;
    });
  return parent;
}

// ----------------------------------------------------------------------------
// Return the node at or below the given root that binds (source, port), or null,
// if no such node exists.
vtkSMSceneGraphNodeProxy* FindOwnerOf(
  vtkSMSceneGraphNodeProxy* root, vtkSMSourceProxy* source, int port, Walk* walk = nullptr)
{
  vtkSMSceneGraphNodeProxy* owner = nullptr;
  const Walk result = VisitSubtree(root,
    [&](vtkSMSceneGraphNodeProxy* current)
    {
      for (const auto& binding : GetBindings(current))
      {
        if (binding.first == source && binding.second == port)
        {
          owner = current;
          return false;
        }
      }
      return true;
    });
  if (walk)
  {
    *walk = result;
  }
  return owner;
}

// ----------------------------------------------------------------------------
// The graph's root as stored, or null. Reads the property rather than calling
// GetRoot(), which would create a root and clear that graph's error code.
vtkSMSceneGraphNodeProxy* StoredRoot(vtkSMSceneGraphProxy* graph)
{
  vtkSMPropertyHelper helper(graph, "Root");
  return helper.GetNumberOfElements() > 0
    ? vtkSMSceneGraphNodeProxy::SafeDownCast(helper.GetAsProxy())
    : nullptr;
}

// ----------------------------------------------------------------------------
// Return true if the given graph targets the given view
bool TargetsView(vtkSMSceneGraphProxy* graph, vtkSMViewProxy* view)
{
  vtkSMPropertyHelper views(graph, "TargetViews");
  for (unsigned int cc = 0; cc < views.GetNumberOfElements(); ++cc)
  {
    if (views.GetAsProxy(cc) == view)
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
const char* NameOf(vtkSMSceneGraphNodeProxy* node)
{
  if (!node)
  {
    return nullptr;
  }
  return vtkSMPropertyHelper(node, "Name").GetAsString();
}

// ----------------------------------------------------------------------------
bool NameEquals(vtkSMSceneGraphNodeProxy* node, const char* name)
{
  const char* actual = NameOf(node);
  return actual && name && strcmp(actual, name) == 0;
}

// ----------------------------------------------------------------------------
// Labels for messages. A source or graph the proxy manager cannot name is
// shown by its XML name or as "?".
std::string NodeLabel(vtkSMSceneGraphNodeProxy* node)
{
  const char* name = NameOf(node);
  return name ? name : "?";
}

// ----------------------------------------------------------------------------
std::string BindingLabel(vtkSMSessionProxyManager* pxm, vtkSMSourceProxy* source, int port)
{
  const char* name = pxm ? pxm->GetProxyName("sources", source) : nullptr;
  return std::string("'") + (name ? name : source->GetXMLName()) + "' port " + std::to_string(port);
}

// ----------------------------------------------------------------------------
std::string GraphLabel(vtkSMSessionProxyManager* pxm, vtkSMSceneGraphProxy* graph)
{
  const char* name =
    pxm ? pxm->GetProxyName(vtkSMSceneGraphProxy::GetSceneGraphGroup(), graph) : nullptr;
  return std::string("scenegraph '") + (name ? name : "?") + "'";
}

//----------------------------------------------------------------------------
// Convenience method to compose a node's local transform with its parent's
// cumulative world transform. Writes the result into the "world" parameter.
void ComposeWorld(vtkMatrix4x4* parentWorld, vtkMatrix4x4* local, vtkMatrix4x4* world)
{
  vtkMatrix4x4::Multiply4x4(parentWorld, local, world);
}

//----------------------------------------------------------------------------
void ReadLocalMatrix(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* matrix)
{
  double values[16];
  vtkSMPropertyHelper(node, "LocalTransform").Get(values, 16);
  matrix->DeepCopy(values);
}

} // anonymous namespace

vtkStandardNewMacro(vtkSMSceneGraphProxy);

//----------------------------------------------------------------------------
vtkSMSceneGraphProxy::vtkSMSceneGraphProxy() = default;

//----------------------------------------------------------------------------
vtkSMSceneGraphProxy::~vtkSMSceneGraphProxy()
{
  this->StopWatchingProxyManager();
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::Fail(ErrorCode code, const std::string& message, bool internalFailure)
{
  this->LastError = code;
  this->LastErrorMessage = message;
  if (internalFailure)
  {
    vtkErrorMacro(<< message);
  }
  else
  {
    vtkWarningMacro(<< message);
  }
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::ClearError()
{
  this->LastError = NoError;
  this->LastErrorMessage.clear();
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::StopWatchingProxyManager()
{
  if (this->WatchedProxyManager)
  {
    if (this->UnRegisterObserverTag)
    {
      this->WatchedProxyManager->RemoveObserver(this->UnRegisterObserverTag);
    }
    if (this->LoadStateObserverTag)
    {
      this->WatchedProxyManager->RemoveObserver(this->LoadStateObserverTag);
    }
  }
  this->WatchedProxyManager = nullptr;
  this->UnRegisterObserverTag = 0;
  this->LoadStateObserverTag = 0;
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::SetSession(vtkSMSession* session)
{
  this->StopWatchingProxyManager();
  this->Superclass::SetSession(session);

  if (auto* pxm = this->GetSessionProxyManager())
  {
    this->WatchedProxyManager = pxm;

    // Watch "proxy unregistered" events in case ParaView deletes a source this
    // graph is controlling, so we can unbind it.
    this->UnRegisterObserverTag = pxm->AddObserver(
      vtkCommand::UnRegisterEvent, this, &vtkSMSceneGraphProxy::OnProxyUnRegistered);

    // Watch "state loaded" events so we can validate the resulting graph
    // structure.
    this->LoadStateObserverTag =
      pxm->AddObserver(vtkCommand::LoadStateEvent, this, &vtkSMSceneGraphProxy::OnStateLoaded);
  }
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::OnStateLoaded(vtkObject*, unsigned long, void*)
{
  this->ValidateStructure();
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::OnProxyUnRegistered(vtkObject* caller, unsigned long, void* callData)
{
  auto* info = reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation*>(callData);
  if (!info || !(info->Type & vtkSMProxyManager::RegisteredProxyInformation::PROXY) ||
    !info->GroupName || std::strcmp(info->GroupName, "sources") != 0)
  {
    // Ignore proxies not in the "sources" group
    return;
  }
  auto* source = vtkSMSourceProxy::SafeDownCast(info->Proxy);
  auto* pxm = vtkSMSessionProxyManager::SafeDownCast(caller);
  if (!source || !pxm)
  {
    return;
  }

  // A rename registers a proxy under the new name before unregistering
  // it under the old one, and unregister first removes the name before
  // triggering this event. So a source that still has a name here is
  // being renamed, not deleted.
  if (pxm->GetProxyName(info->GroupName, source))
  {
    return;
  }
  this->UnbindSource(source);
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::UnbindSource(vtkSMSourceProxy* source)
{
  if (!source)
  {
    return;
  }

  // Deliberately no transform reset here, unlike the other unbinding paths.
  // Since the source is on its way out of the pipeline, there is nothing left
  // to return to a default position.
  VisitSubtree(this->GetRoot(),
    [source](vtkSMSceneGraphNodeProxy* current)
    {
      std::vector<Binding> bindings = GetBindings(current);
      const size_t before = bindings.size();
      bindings.erase(std::remove_if(bindings.begin(), bindings.end(),
                       [source](const Binding& b) { return b.first == source; }),
        bindings.end());
      if (bindings.size() != before)
      {
        SetBindings(current, bindings);
      }
      return true;
    });
}

//----------------------------------------------------------------------------
vtkSMSceneGraphProxy* vtkSMSceneGraphProxy::CreateSceneGraph(
  vtkSMSessionProxyManager* pxm, const char* name)
{
  if (!pxm)
  {
    vtkGenericWarningMacro("CreateSceneGraph requires a session proxy manager.");
    return nullptr;
  }

  vtkSMProxy* proxy = pxm->NewProxy(vtkSMSceneGraphProxy::GetDefinitionGroup(), "SceneGraph");
  vtkSMSceneGraphProxy* graph = vtkSMSceneGraphProxy::SafeDownCast(proxy);
  if (!graph)
  {
    vtkGenericWarningMacro("Failed to create a SceneGraph proxy. Is the SceneGraph plugin loaded?");
    if (proxy)
    {
      proxy->Delete();
    }
    return nullptr;
  }

  graph->UpdateVTKObjects();
  const char* group = vtkSMSceneGraphProxy::GetSceneGraphGroup();
  if (name && name[0])
  {
    // RegisterProxy(group, name, proxy) tolerates duplicate names, so uniquify here.
    // We pass `alwaysAppend`=false, otherwise it adds a number to the name regardless
    // of whether the name is already taken.
    pxm->RegisterProxy(group, pxm->GetUniqueProxyName(group, name, false).c_str(), graph);
  }
  else
  {
    pxm->RegisterProxy(group, graph);
  }

  // Make sure the root exists from the moment the graph does, so callers can never
  // get their hands on an empty graph.
  graph->GetRoot();

  // All graphs are created targeting the active render view, if there is one.
  if (auto* selection = pxm->GetSelectionModel("ActiveView"))
  {
    if (auto* active = vtkSMRenderViewProxy::SafeDownCast(selection->GetCurrentProxy()))
    {
      graph->AddTargetView(active);
    }
  }

  // RegisterProxy() documentation says it takes control of the proxy and
  // after invoking it, we can delete our reference count.
  graph->Delete();
  return graph;
}

//----------------------------------------------------------------------------
vtkSMSceneGraphProxy* vtkSMSceneGraphProxy::FindSceneGraph(
  vtkSMSessionProxyManager* pxm, const char* name)
{
  if (!pxm || !name)
  {
    return nullptr;
  }
  return vtkSMSceneGraphProxy::SafeDownCast(
    pxm->GetProxy(vtkSMSceneGraphProxy::GetSceneGraphGroup(), name));
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::GetSceneGraphs(vtkSMSessionProxyManager* pxm, vtkCollection* graphs)
{
  if (!graphs)
  {
    return;
  }

  graphs->RemoveAllItems();

  if (!pxm)
  {
    return;
  }

  const char* group = vtkSMSceneGraphProxy::GetSceneGraphGroup();
  const unsigned int count = pxm->GetNumberOfProxies(group);
  for (unsigned int cc = 0; cc < count; ++cc)
  {
    const char* name = pxm->GetProxyName(group, cc);
    if (auto* graph = vtkSMSceneGraphProxy::SafeDownCast(pxm->GetProxy(group, name)))
    {
      graphs->AddItem(graph);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::DeleteSceneGraph(
  vtkSMSessionProxyManager* pxm, vtkSMSceneGraphProxy* graph)
{
  if (!graph)
  {
    vtkGenericWarningMacro("DeleteSceneGraph requires a graph.");
    return false;
  }

  graph->ClearError();

  if (!pxm)
  {
    graph->Fail(NullArgument, "DeleteSceneGraph requires a proxy manager.");
    return false;
  }

  // UnregisterSubtree() resets any bound sources graph nodes may have been controlling.
  if (auto* root = graph->GetRoot())
  {
    graph->UnregisterSubtree(root);
  }

  const char* group = vtkSMSceneGraphProxy::GetSceneGraphGroup();
  const char* name = pxm->GetProxyName(group, graph);

  if (!name)
  {
    graph->Fail(NotRegistered, "This graph is not registered, so it cannot be deleted.");
    return false;
  }

  pxm->UnRegisterProxy(group, name, graph);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::RenameSceneGraph(
  vtkSMSessionProxyManager* pxm, vtkSMSceneGraphProxy* graph, const char* name)
{
  if (!graph)
  {
    vtkGenericWarningMacro("RenameSceneGraph requires a graph.");
    return false;
  }

  graph->ClearError();

  if (!pxm || !name || !name[0])
  {
    graph->Fail(NullArgument, "RenameSceneGraph requires a proxy manager and a non-empty name.");
    return false;
  }

  const char* group = vtkSMSceneGraphProxy::GetSceneGraphGroup();
  const char* registered = pxm->GetProxyName(group, graph);

  if (!registered)
  {
    graph->Fail(NotRegistered, "This graph is not registered, so it has no name to change.");
    return false;
  }

  const std::string oldName = registered;

  if (oldName == name)
  {
    return true;
  }

  pxm->RegisterProxy(group, pxm->GetUniqueProxyName(group, name, false).c_str(), graph);
  pxm->UnRegisterProxy(group, oldName.c_str(), graph);
  return true;
}

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* vtkSMSceneGraphProxy::CreateNode(const char* name)
{
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  if (!pxm)
  {
    this->Fail(CreationFailed, "No session proxy manager; cannot create a node.", true);
    return nullptr;
  }

  vtkSMProxy* proxy = pxm->NewProxy(vtkSMSceneGraphProxy::GetDefinitionGroup(), "SceneGraphNode");
  vtkSMSceneGraphNodeProxy* node = vtkSMSceneGraphNodeProxy::SafeDownCast(proxy);

  if (!node)
  {
    this->Fail(CreationFailed, "Failed to create a SceneGraphNode proxy.", true);
    if (proxy)
    {
      proxy->Delete();
    }
    return nullptr;
  }

  if (name && name[0])
  {
    vtkSMPropertyHelper(node, "Name").Set(name);
  }

  node->UpdateVTKObjects();
  pxm->RegisterProxy(vtkSMSceneGraphProxy::GetNodeGroup(), node);

  // RegisterProxy() documentation says it takes control of the proxy and
  // after invoking it, we can delete our reference count.
  node->Delete();
  return node;
}

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* vtkSMSceneGraphProxy::GetRoot()
{
  this->ClearError();
  vtkSMPropertyHelper rootHelper(this, "Root");

  if (rootHelper.GetNumberOfElements() > 0)
  {
    if (auto* existing = vtkSMSceneGraphNodeProxy::SafeDownCast(rootHelper.GetAsProxy()))
    {
      return existing;
    }
  }

  vtkSMSceneGraphNodeProxy* root = this->CreateNode("World");
  if (!root)
  {
    return nullptr;
  }

  rootHelper.Set(root);
  this->UpdateVTKObjects();
  return root;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::CollectAncestors(
  vtkSMSceneGraphNodeProxy* node, std::vector<vtkSMSceneGraphNodeProxy*>& chain)
{
  chain.clear();
  if (!node)
  {
    this->Fail(NullArgument, "A node is required.");
    return false;
  }

  // Walk down from the root, recording each node's parent along the way, until
  // either 1) the requested node is reached, 2) a node is found to have a second
  // parent, or 3) the search is exhausted.
  vtkSMSceneGraphNodeProxy* root = this->GetRoot();
  std::unordered_map<vtkSMSceneGraphNodeProxy*, vtkSMSceneGraphNodeProxy*> parentOf;
  bool found = false;
  vtkSMSceneGraphNodeProxy* repeated = nullptr;
  const Walk walk = VisitSubtree(root,
    [&](vtkSMSceneGraphNodeProxy* current)
    {
      if (current == node)
      {
        found = true;
        return false;
      }
      for (auto* child : GetChildVector(current))
      {
        if (!parentOf.emplace(child, current).second)
        {
          repeated = child;
          return false;
        }
      }
      return true;
    });
  if (!repeated)
  {
    // If we didn't detect a repeat here, the visit method may have (e.g. the root
    // listed as a child of some other node)
    repeated = walk.Repeated;
  }
  if (repeated)
  {
    this->Fail(NotATree, NotATreeMessage(repeated));
    return false;
  }
  if (!found)
  {
    this->Fail(NotInGraph, "Given node is not part of this scenegraph.");
    return false;
  }

  // Now build the ancestors chain, by looking up parents in the parents map,
  // starting with the target node, and ending when we get to the root.
  for (vtkSMSceneGraphNodeProxy* current = node; current != root;
       current = parentOf.find(current)->second)
  {
    chain.push_back(current);
  }

  chain.push_back(root);
  return true;
}

//----------------------------------------------------------------------------
const char* vtkSMSceneGraphProxy::GetNodeName(vtkSMSceneGraphNodeProxy* node)
{
  this->ClearError();
  if (!node)
  {
    this->Fail(NullArgument, "GetNodeName requires a node.");
    return nullptr;
  }
  return NameOf(node);
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::Contains(vtkSMSceneGraphNodeProxy* node)
{
  if (!node)
  {
    return false;
  }

  // Every node is attached to something. If the given node is in this graph,
  // it's either the root node, or else some other node lists it as a child.
  vtkSMSceneGraphNodeProxy* root = this->GetRoot();
  if (node == root)
  {
    return true;
  }

  Walk walk;
  const bool contained = FindParentOf(root, node, walk) != nullptr;

  if (walk.Repeated)
  {
    this->Fail(NotATree, NotATreeMessage(walk.Repeated));
    return false;
  }

  return contained;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::RequireContains(vtkSMSceneGraphNodeProxy* node)
{
  if (this->Contains(node))
  {
    return true;
  }

  // Contains() has already reported NotATree, if encountered, in which case that
  // is the error the user should see. If we're here and the error is not set,
  // fail because the required node is not in this graph.
  if (this->LastError == NoError)
  {
    this->Fail(NotInGraph, "Given node is not part of this scenegraph.");
  }

  return false;
}

//----------------------------------------------------------------------------
std::string vtkSMSceneGraphProxy::SanitizeNodeName(const char* name)
{
  std::string sanitized = (name && name[0]) ? name : "Node";
  std::replace(sanitized.begin(), sanitized.end(), '/', '_');
  return sanitized;
}

//----------------------------------------------------------------------------
unsigned int vtkSMSceneGraphProxy::GetNumberOfChildren(vtkSMSceneGraphNodeProxy* node)
{
  this->ClearError();
  if (!node)
  {
    this->Fail(NullArgument, "GetNumberOfChildren requires a node.");
    return 0;
  }
  return static_cast<unsigned int>(GetChildVector(node).size());
}

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* vtkSMSceneGraphProxy::GetChild(
  vtkSMSceneGraphNodeProxy* node, unsigned int index)
{
  this->ClearError();
  if (!node)
  {
    this->Fail(NullArgument, "GetChild requires a node.");
    return nullptr;
  }
  const std::vector<vtkSMSceneGraphNodeProxy*> children = GetChildVector(node);
  if (index >= children.size())
  {
    this->Fail(OutOfRange, "Child index " + std::to_string(index) + " is out of range.");
    return nullptr;
  }
  return children[index];
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::GetChildren(vtkSMSceneGraphNodeProxy* node, vtkCollection* children)
{
  this->ClearError();
  if (!children)
  {
    this->Fail(NullArgument, "GetChildren requires a collection to fill.");
    return;
  }
  children->RemoveAllItems();
  if (!node)
  {
    this->Fail(NullArgument, "GetChildren requires a node.");
    return;
  }
  for (auto* child : GetChildVector(node))
  {
    children->AddItem(child);
  }
}

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* vtkSMSceneGraphProxy::GetParent(vtkSMSceneGraphNodeProxy* node)
{
  this->ClearError();
  vtkSMSceneGraphNodeProxy* root = this->GetRoot();

  if (!node)
  {
    this->Fail(NullArgument, "GetParent requires a node.");
    return nullptr;
  }

  if (node == root)
  {
    return nullptr; // the root has no parent
  }

  Walk walk;
  vtkSMSceneGraphNodeProxy* parent = FindParentOf(root, node, walk);

  if (walk.Repeated)
  {
    this->Fail(NotATree, NotATreeMessage(walk.Repeated));
    return nullptr;
  }

  if (!parent)
  {
    this->Fail(NotInGraph, "Given node is not part of this scenegraph.");
  }

  return parent;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::IsDescendantOf(
  vtkSMSceneGraphNodeProxy* node, vtkSMSceneGraphNodeProxy* ancestor)
{
  this->ClearError();
  if (!node || !ancestor)
  {
    this->Fail(NullArgument, "IsDescendantOf requires a node and an ancestor.");
    return false;
  }

  if (node == ancestor)
  {
    return false;
  }

  const Walk walk = VisitSubtree(ancestor,
    [node](vtkSMSceneGraphNodeProxy* current)
    {
      for (auto* child : GetChildVector(current))
      {
        if (child == node)
        {
          // stopping the walk indicates success
          return false;
        }
      }
      return true;
    });

  if (walk.Repeated)
  {
    this->Fail(NotATree, NotATreeMessage(walk.Repeated));
    return false;
  }

  return walk.Stopped;
}

//----------------------------------------------------------------------------
int vtkSMSceneGraphProxy::GetDepth(vtkSMSceneGraphNodeProxy* node)
{
  this->ClearError();
  std::vector<vtkSMSceneGraphNodeProxy*> chain;

  if (!this->CollectAncestors(node, chain))
  {
    return -1;
  }

  // Subtract one from the number of nodes in the chain, since we're returning
  // "the number of edges from the node to the root".
  return static_cast<int>(chain.size()) - 1;
}

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* vtkSMSceneGraphProxy::FindChild(
  vtkSMSceneGraphNodeProxy* parent, const char* name)
{
  this->ClearError();
  if (!parent || !name)
  {
    this->Fail(NullArgument, "FindChild requires a parent and a name.");
    return nullptr;
  }
  for (auto* child : GetChildVector(parent))
  {
    if (NameEquals(child, name))
    {
      return child;
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* vtkSMSceneGraphProxy::FindNode(const char* path)
{
  this->ClearError();
  if (!path)
  {
    this->Fail(NullArgument, "FindNode requires a path.");
    return nullptr;
  }
  vtkSMSceneGraphNodeProxy* current = this->GetRoot();
  if (!current)
  {
    return nullptr;
  }

  std::vector<std::string> components;
  std::istringstream stream(path);
  std::string component;
  while (std::getline(stream, component, '/'))
  {
    if (!component.empty())
    {
      components.push_back(component);
    }
  }

  if (components.empty() || !NameEquals(current, components[0].c_str()))
  {
    return nullptr;
  }

  for (size_t cc = 1; cc < components.size(); ++cc)
  {
    current = this->FindChild(current, components[cc].c_str());
    if (!current)
    {
      return nullptr;
    }
  }
  return current;
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::GetNodesByName(const char* name, vtkCollection* nodes)
{
  this->ClearError();
  if (!nodes)
  {
    this->Fail(NullArgument, "GetNodesByName requires a collection to fill.");
    return;
  }

  nodes->RemoveAllItems();

  if (!name)
  {
    this->Fail(NullArgument, "GetNodesByName requires a name.");
    return;
  }

  const Walk walk = VisitSubtree(this->GetRoot(),
    [name, nodes](vtkSMSceneGraphNodeProxy* current)
    {
      if (NameEquals(current, name))
      {
        nodes->AddItem(current);
      }
      return true;
    });

  if (walk.Repeated)
  {
    this->Fail(NotATree, NotATreeMessage(walk.Repeated));
  }
}

//----------------------------------------------------------------------------
std::string vtkSMSceneGraphProxy::GetPath(vtkSMSceneGraphNodeProxy* node)
{
  this->ClearError();
  std::vector<vtkSMSceneGraphNodeProxy*> chain;

  if (!this->CollectAncestors(node, chain))
  {
    return std::string();
  }

  std::string path;

  for (auto iter = chain.rbegin(); iter != chain.rend(); ++iter)
  {
    if (!path.empty())
    {
      path += "/";
    }
    const char* name = NameOf(*iter);
    path += name ? name : "";
  }

  return path;
}

//----------------------------------------------------------------------------
std::string vtkSMSceneGraphProxy::MakeUniqueChildName(
  vtkSMSceneGraphNodeProxy* parent, const char* name, vtkSMSceneGraphNodeProxy* exclude)
{
  const std::string base = vtkSMSceneGraphProxy::SanitizeNodeName(name);

  std::vector<std::string> taken;
  for (auto* child : GetChildVector(parent))
  {
    if (child == exclude)
    {
      continue;
    }
    if (const char* childName = NameOf(child))
    {
      taken.emplace_back(childName);
    }
  }

  auto isTaken = [&taken](const std::string& candidate)
  { return std::find(taken.begin(), taken.end(), candidate) != taken.end(); };

  if (!isTaken(base))
  {
    return base;
  }

  for (int suffix = 1;; ++suffix)
  {
    const std::string candidate = base + std::to_string(suffix);
    if (!isTaken(candidate))
    {
      return candidate;
    }
  }
}

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* vtkSMSceneGraphProxy::CreateChild(
  vtkSMSceneGraphNodeProxy* parent, const char* name)
{
  this->ClearError();
  if (!parent)
  {
    this->Fail(NullArgument, "CreateChild requires a parent node.");
    return nullptr;
  }
  if (!this->RequireContains(parent))
  {
    return nullptr;
  }
  vtkSMSceneGraphNodeProxy* child = this->CreateNode(name);
  if (!child)
  {
    return nullptr;
  }

  // Nothing below here can fail, which is what keeps a node from ending up
  // registered without a parent: either it is created and attached, or it was
  // never created at all.
  vtkSMPropertyHelper(child, "Name")
    .Set(this->MakeUniqueChildName(parent, NameOf(child), nullptr).c_str());
  child->UpdateVTKObjects();

  std::vector<vtkSMSceneGraphNodeProxy*> children = GetChildVector(parent);
  children.push_back(child);
  SetChildVector(parent, children);
  return child;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::MoveNode(
  vtkSMSceneGraphNodeProxy* node, vtkSMSceneGraphNodeProxy* newParent)
{
  this->ClearError();
  if (!node || !newParent)
  {
    this->Fail(NullArgument, "MoveNode requires both a node and a new parent.");
    return false;
  }
  if (node == this->GetRoot())
  {
    this->Fail(RootIsFixed, "The root node cannot be moved.");
    return false;
  }
  if (node == newParent || this->IsDescendantOf(newParent, node))
  {
    this->Fail(WouldFormCycle, "Cannot move a node under itself or a descendant.");
    return false;
  }
  // IsDescendantOf() sets NotATree error, if encountered. In that case NotATree
  // is the error the user should see, so just return false.
  if (this->LastError != NoError)
  {
    return false;
  }
  if (!this->RequireContains(newParent))
  {
    return false;
  }

  // GetParent() reports NotInGraph and NotATree errors, if encountered
  vtkSMSceneGraphNodeProxy* oldParent = this->GetParent(node);
  if (!oldParent)
  {
    return false;
  }

  // The node is already a child of the newParent
  if (newParent == oldParent)
  {
    return true;
  }

  std::vector<vtkSMSceneGraphNodeProxy*> oldChildren = GetChildVector(oldParent);
  oldChildren.erase(std::remove(oldChildren.begin(), oldChildren.end(), node), oldChildren.end());
  SetChildVector(oldParent, oldChildren);

  vtkSMPropertyHelper(node, "Name")
    .Set(this->MakeUniqueChildName(newParent, NameOf(node), nullptr).c_str());
  node->UpdateVTKObjects();

  std::vector<vtkSMSceneGraphNodeProxy*> newChildren = GetChildVector(newParent);
  newChildren.push_back(node);
  SetChildVector(newParent, newChildren);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::RenameNode(vtkSMSceneGraphNodeProxy* node, const char* name)
{
  this->ClearError();
  if (!node || !name || !name[0])
  {
    this->Fail(NullArgument, "RenameNode requires a node and a non-empty name.");
    return false;
  }
  if (std::strchr(name, '/'))
  {
    this->Fail(NameHasSeparator,
      std::string("A node name cannot contain '/', which separates the components of a path: '") +
        name + "' would give this node a path that cannot be looked up again.");
    return false;
  }
  // The root has no siblings, so only other nodes need a name check
  if (node != this->GetRoot())
  {
    // GetParent() reports NotInGraph and NotATree errors, if encountered
    vtkSMSceneGraphNodeProxy* parent = this->GetParent(node);
    if (!parent)
    {
      return false;
    }

    if (this->MakeUniqueChildName(parent, name, node) != name)
    {
      this->Fail(NameTaken, std::string("A sibling is already named '") + name + "'.");
      return false;
    }
  }

  vtkSMPropertyHelper(node, "Name").Set(name);
  node->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::DeleteNode(vtkSMSceneGraphNodeProxy* node)
{
  this->ClearError();
  if (!node)
  {
    this->Fail(NullArgument, "DeleteNode requires a node.");
    return false;
  }
  if (node == this->GetRoot())
  {
    this->Fail(RootIsFixed, "The root node cannot be deleted.");
    return false;
  }

  // GetParent() reports NotInGraph and NotATree errors, if encountered
  vtkSMSceneGraphNodeProxy* parent = this->GetParent(node);
  if (!parent)
  {
    return false;
  }

  std::vector<vtkSMSceneGraphNodeProxy*> orphans = GetChildVector(node);

  // Splice the children into the parent at the position the node occupied, so
  // sibling order is preserved. Leave their local transforms alone, so they
  // shift by whatever the deleted node's transform contributed.
  std::vector<vtkSMSceneGraphNodeProxy*> siblings = GetChildVector(parent);
  auto position = std::find(siblings.begin(), siblings.end(), node);
  const size_t at = static_cast<size_t>(std::distance(siblings.begin(), position));
  if (position != siblings.end())
  {
    siblings.erase(position);
  }
  siblings.insert(siblings.begin() + std::min(at, siblings.size()), orphans.begin(), orphans.end());
  SetChildVector(parent, siblings);

  // The parent now references the orphans, so the node can safely let go.
  SetChildVector(node, {});

  // Names only have to be unique among siblings, and these just changed siblings.
  for (auto* orphan : orphans)
  {
    vtkSMPropertyHelper(orphan, "Name")
      .Set(this->MakeUniqueChildName(parent, NameOf(orphan), orphan).c_str());
    orphan->UpdateVTKObjects();
  }

  this->UnregisterSubtree(node);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::RemoveSubtree(vtkSMSceneGraphNodeProxy* node)
{
  this->ClearError();
  if (!node)
  {
    this->Fail(NullArgument, "RemoveSubtree requires a node.");
    return false;
  }
  if (node == this->GetRoot())
  {
    this->Fail(RootIsFixed, "The root node cannot be removed.");
    return false;
  }

  // GetParent() reports NotInGraph and NotATree errors, if encountered
  vtkSMSceneGraphNodeProxy* parent = this->GetParent(node);
  if (!parent)
  {
    return false;
  }

  std::vector<vtkSMSceneGraphNodeProxy*> siblings = GetChildVector(parent);
  siblings.erase(std::remove(siblings.begin(), siblings.end(), node), siblings.end());
  SetChildVector(parent, siblings);

  this->UnregisterSubtree(node);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::UnregisterSubtree(vtkSMSceneGraphNodeProxy* node)
{
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  if (!node || !pxm)
  {
    return;
  }

  // Collect the whole subtree before removing any children
  std::vector<vtkSMSceneGraphNodeProxy*> doomed;
  VisitSubtree(node,
    [&doomed](vtkSMSceneGraphNodeProxy* current)
    {
      doomed.push_back(current);
      return true;
    });

  // Iterate over the collected nodes from the subtree and, for each one,
  // reset its bindings, clear its child list, and unregister the node.
  const char* group = vtkSMSceneGraphProxy::GetNodeGroup();
  for (auto* victim : doomed)
  {
    this->ResetBindingsOf(victim);
    SetChildVector(victim, {});
    if (const char* name = pxm->GetProxyName(group, victim))
    {
      pxm->UnRegisterProxy(group, name, victim);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::SetLocalTransform(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* matrix)
{
  this->ClearError();
  if (!node || !matrix)
  {
    this->Fail(NullArgument, "SetLocalTransform requires a node and a matrix.");
    return false;
  }
  if (!this->RequireContains(node))
  {
    return false;
  }

  double values[16];
  vtkMatrix4x4::DeepCopy(values, matrix);
  vtkSMPropertyHelper(node, "LocalTransform").Set(values, 16);
  node->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::GetLocalTransform(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* matrix)
{
  this->ClearError();
  if (!node || !matrix)
  {
    this->Fail(NullArgument, "GetLocalTransform requires a node and a matrix.");
    return false;
  }
  ReadLocalMatrix(node, matrix);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::GetWorldTransform(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* matrix)
{
  this->ClearError();
  if (!node || !matrix)
  {
    this->Fail(NullArgument, "GetWorldTransform requires a node and a matrix.");
    return false;
  }

  // The chain is ordered from immediate parent through root, so composing the
  // given node's transform with the one accumulated through its ancestors, requires
  // iterating through it backwards.
  std::vector<vtkSMSceneGraphNodeProxy*> chain;
  if (!this->CollectAncestors(node, chain))
  {
    return false;
  }

  matrix->Identity();
  vtkNew<vtkMatrix4x4> local;
  vtkNew<vtkMatrix4x4> composed;
  for (auto iter = chain.rbegin(); iter != chain.rend(); ++iter)
  {
    ReadLocalMatrix(*iter, local);
    ComposeWorld(matrix, local, composed);
    matrix->DeepCopy(composed);
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::AddSource(
  vtkSMSceneGraphNodeProxy* node, vtkSMSourceProxy* source, int port)
{
  this->ClearError();
  if (!node || !source)
  {
    this->Fail(NullArgument, "AddSource requires a node and a source.");
    return false;
  }
  if (port < 0)
  {
    this->Fail(OutOfRange, "Output port " + std::to_string(port) + " is not a valid port.");
    return false;
  }
  if (!this->RequireContains(node))
  {
    return false;
  }

  // At most one node drives a given (source, port): two nodes writing one
  // representation's transform would simply overwrite each other.
  vtkSMSceneGraphNodeProxy* owner = this->GetOwningNode(source, port);
  if (owner == node)
  {
    return true; // already bound here
  }

  // Across graphs the binding is refused rather than taken, as taking it would
  // mean editing the other graph.
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  vtkSMPropertyHelper targets(this, "TargetViews");
  for (unsigned int cc = 0; cc < targets.GetNumberOfElements(); ++cc)
  {
    auto* view = vtkSMViewProxy::SafeDownCast(targets.GetAsProxy(cc));
    vtkSMSceneGraphNodeProxy* holder = nullptr;
    if (auto* other = view ? this->FindOtherBinding(source, port, view, holder) : nullptr)
    {
      this->Fail(BindingConflict,
        BindingLabel(pxm, source, port) + " is already bound to node '" + NodeLabel(holder) +
          "' of " + GraphLabel(pxm, other) + ", which targets a view this graph targets.");
      return false;
    }
  }

  if (owner)
  {
    this->RemoveSource(owner, source, port);
  }

  std::vector<Binding> bindings = GetBindings(node);
  bindings.emplace_back(source, port);
  SetBindings(node, bindings);
  return true;
}

//----------------------------------------------------------------------------
vtkSMSceneGraphProxy* vtkSMSceneGraphProxy::FindOtherBinding(
  vtkSMSourceProxy* source, int port, vtkSMViewProxy* view, vtkSMSceneGraphNodeProxy*& node)
{
  node = nullptr;
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  if (!pxm)
  {
    return nullptr;
  }
  vtkNew<vtkCollection> graphs;
  vtkSMSceneGraphProxy::GetSceneGraphs(pxm, graphs);
  for (int cc = 0; cc < graphs->GetNumberOfItems(); ++cc)
  {
    auto* other = vtkSMSceneGraphProxy::SafeDownCast(graphs->GetItemAsObject(cc));
    if (!other || other == this || !TargetsView(other, view))
    {
      continue;
    }
    // Reads the other graphs' properties directly, to avoid changing
    // their error codes.
    if (auto* holder = FindOwnerOf(StoredRoot(other), source, port))
    {
      node = holder;
      return other;
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::RemoveSource(
  vtkSMSceneGraphNodeProxy* node, vtkSMSourceProxy* source, int port)
{
  this->ClearError();
  if (!node || !source)
  {
    this->Fail(NullArgument, "RemoveSource requires a node and a source.");
    return false;
  }

  std::vector<Binding> bindings = GetBindings(node);
  const size_t before = bindings.size();
  bindings.erase(
    std::remove(bindings.begin(), bindings.end(), Binding(source, port)), bindings.end());
  if (bindings.size() == before)
  {
    this->Fail(NotBound, "That source and port are not bound to this node.");
    return false;
  }
  SetBindings(node, bindings);

  // Put whatever this source draws back where it started. Nothing is driving
  // it any more, and unbinding is the only way a user has to undo a placement.
  this->ResetBinding(source, port);
  return true;
}

//----------------------------------------------------------------------------
unsigned int vtkSMSceneGraphProxy::GetNumberOfSources(vtkSMSceneGraphNodeProxy* node)
{
  this->ClearError();
  if (!node)
  {
    this->Fail(NullArgument, "GetNumberOfSources requires a node.");
    return 0;
  }
  return static_cast<unsigned int>(GetBindings(node).size());
}

//----------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMSceneGraphProxy::GetSource(
  vtkSMSceneGraphNodeProxy* node, unsigned int index)
{
  this->ClearError();
  if (!node)
  {
    this->Fail(NullArgument, "GetSource requires a node.");
    return nullptr;
  }
  const std::vector<Binding> bindings = GetBindings(node);
  if (index >= bindings.size())
  {
    this->Fail(OutOfRange, "Binding index " + std::to_string(index) + " is out of range.");
    return nullptr;
  }
  return bindings[index].first;
}

//----------------------------------------------------------------------------
int vtkSMSceneGraphProxy::GetSourcePort(vtkSMSceneGraphNodeProxy* node, unsigned int index)
{
  this->ClearError();
  if (!node)
  {
    this->Fail(NullArgument, "GetSourcePort requires a node.");
    return -1;
  }
  const std::vector<Binding> bindings = GetBindings(node);
  if (index >= bindings.size())
  {
    this->Fail(OutOfRange, "Binding index " + std::to_string(index) + " is out of range.");
    return -1;
  }
  return bindings[index].second;
}

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* vtkSMSceneGraphProxy::GetOwningNode(vtkSMSourceProxy* source, int port)
{
  this->ClearError();
  if (!source)
  {
    this->Fail(NullArgument, "GetOwningNode requires a source.");
    return nullptr;
  }

  Walk walk;
  vtkSMSceneGraphNodeProxy* owner = FindOwnerOf(this->GetRoot(), source, port, &walk);
  if (walk.Repeated)
  {
    this->Fail(NotATree, NotATreeMessage(walk.Repeated));
    return nullptr;
  }
  return owner;
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::GetDrivenRepresentations(
  vtkSMSceneGraphNodeProxy* node, vtkSMViewProxy* view, vtkCollection* reps)
{
  this->ClearError();
  if (!reps)
  {
    this->Fail(NullArgument, "GetDrivenRepresentations requires a collection to fill.");
    return;
  }
  reps->RemoveAllItems();
  if (!node || !view)
  {
    this->Fail(NullArgument, "GetDrivenRepresentations requires a node and a view.");
    return;
  }
  for (const auto& binding : GetBindings(node))
  {
    if (auto* rep = view->FindRepresentation(binding.first, binding.second))
    {
      reps->AddItem(rep);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::AddTargetView(vtkSMViewProxy* view)
{
  this->ClearError();
  if (!view)
  {
    this->Fail(NullArgument, "AddTargetView requires a view.");
    return false;
  }
  if (!vtkSMRenderViewProxy::SafeDownCast(view))
  {
    this->Fail(NotARenderView,
      std::string("A scenegraph drives 3D render views only; ignoring a ") + view->GetXMLName() +
        ".");
    return false;
  }

  vtkSMPropertyHelper views(this, "TargetViews");
  for (unsigned int cc = 0; cc < views.GetNumberOfElements(); ++cc)
  {
    if (views.GetAsProxy(cc) == view)
    {
      return true; // already targeted
    }
  }

  // Each binding this graph holds must be free in the new view, or two
  // graphs would take turns writing the same representation.
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  bool refused = false;
  VisitSubtree(this->GetRoot(),
    [&](vtkSMSceneGraphNodeProxy* current)
    {
      for (const auto& binding : GetBindings(current))
      {
        vtkSMSceneGraphNodeProxy* holder = nullptr;
        if (auto* other = this->FindOtherBinding(binding.first, binding.second, view, holder))
        {
          this->Fail(BindingConflict,
            "This view cannot be targeted: " + BindingLabel(pxm, binding.first, binding.second) +
              " is already bound to node '" + NodeLabel(holder) + "' of " + GraphLabel(pxm, other) +
              ", which targets it.");
          refused = true;
          return false;
        }
      }
      return true;
    });
  if (refused)
  {
    return false;
  }

  views.Add(view);
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::RemoveTargetView(vtkSMViewProxy* view)
{
  this->ClearError();
  if (!view)
  {
    this->Fail(NullArgument, "RemoveTargetView requires a view.");
    return false;
  }
  vtkSMPropertyHelper views(this, "TargetViews");
  for (unsigned int cc = 0; cc < views.GetNumberOfElements(); ++cc)
  {
    if (views.GetAsProxy(cc) == view)
    {
      // Reset while the view is still a target, so ResetBinding can still see
      // it. Otherwise everything in it would be left displaced with nothing
      // driving it.
      const Walk walk = VisitSubtree(this->GetRoot(),
        [this](vtkSMSceneGraphNodeProxy* current)
        {
          this->ResetBindingsOf(current);
          return true;
        });
      if (walk.Repeated)
      {
        this->Fail(NotATree, NotATreeMessage(walk.Repeated));
      }

      views.Remove(view);
      this->UpdateVTKObjects();
      return true;
    }
  }
  this->Fail(NotBound, "That view is not a target of this scenegraph.");
  return false;
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::GetTargetViews(vtkCollection* views)
{
  this->ClearError();
  if (!views)
  {
    this->Fail(NullArgument, "GetTargetViews requires a collection to fill.");
    return;
  }
  views->RemoveAllItems();
  vtkSMPropertyHelper helper(this, "TargetViews");
  for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); ++cc)
  {
    if (auto* view = vtkSMViewProxy::SafeDownCast(helper.GetAsProxy(cc)))
    {
      views->AddItem(view);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkSMSceneGraphProxy::ValidateStructure()
{
  vtkSMSceneGraphNodeProxy* root = this->GetRoot();
  if (!root)
  {
    return false; // GetRoot() has already said why
  }

  // Warn about every defect as it is found. The error code reports only the
  // most severe one. Structural issues are considered most severe, then
  // addressing problems, then issues with bindings, where a conflict with
  // another graph outranks a binding to something that is not a source.
  ErrorCode worst = NoError;
  int worstRank = 0;
  std::string worstMessage;
  auto flag = [&](ErrorCode code, int rank, const std::string& message)
  {
    vtkWarningMacro(<< message);
    if (rank > worstRank)
    {
      worst = code;
      worstRank = rank;
      worstMessage = message;
    }
  };
  auto nameOf = NodeLabel;
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  vtkSMPropertyHelper targets(this, "TargetViews");

  // Walk what is reachable, counting how many child lists name each node. In a
  // tree the root should be named by no other nodes, while all other nodes should
  // be named by exactly one.
  std::map<vtkSMSceneGraphNodeProxy*, int> parentCount;
  VisitSubtree(root,
    [&](vtkSMSceneGraphNodeProxy* current)
    {
      // Don't use GetChildVector() here, it filters anything that isn't a
      // vtkSMSceneGraphNodeProxy, which is one of the things we want to know
      // about.
      if (auto* children = vtkSMProxyProperty::SafeDownCast(current->GetProperty("Children")))
      {
        std::set<std::string> namesSeen;
        const unsigned int count = children->GetNumberOfProxies();
        for (unsigned int cc = 0; cc < count; ++cc)
        {
          vtkSMProxy* entry = children->GetProxy(cc);
          auto* child = vtkSMSceneGraphNodeProxy::SafeDownCast(entry);
          if (!child)
          {
            flag(NotANode, 5,
              "Node '" + nameOf(current) + "' lists a " + (entry ? entry->GetXMLName() : "null") +
                " among its children, which is not a scenegraph node.");
            continue;
          }
          parentCount[child] += 1;

          // "/" is not allowed in node names, which is enforced by the graph api,
          // but direct property edits could circumvent that, so check it here.
          const char* childName = NameOf(child);
          if (childName && std::strchr(childName, '/'))
          {
            flag(NameHasSeparator, 3,
              "Node '" + nameOf(current) + "' has a child named '" + childName +
                "', and '/' is not allowed in a node name.");
          }

          // Siblings must differ, or else paths are ambiguous. RenameNode() enforces
          // this, but direct property edits could circumvent that, so check it here.
          if (!namesSeen.insert(childName ? childName : "").second)
          {
            flag(NameTaken, 4,
              "Node '" + nameOf(current) + "' has more than one child named '" +
                (childName ? childName : "") + "'. Sibling names must differ.");
          }
        }
      }

      // Check for bindings that are to something that is not a source proxy
      if (auto* sources = vtkSMInputProperty::SafeDownCast(current->GetProperty("Sources")))
      {
        const unsigned int count = sources->GetNumberOfProxies();
        for (unsigned int cc = 0; cc < count; ++cc)
        {
          vtkSMProxy* entry = sources->GetProxy(cc);
          if (entry && !vtkSMSourceProxy::SafeDownCast(entry))
          {
            flag(NotASource, 1,
              "Node '" + nameOf(current) + "' is bound to a " + entry->GetXMLName() +
                ", which is not a pipeline source.");
          }
        }
      }

      // A binding conflict can't be created by AddSource() or AddTargetView(), but
      // other avenues, such as a direct property edit, can still cause one.
      for (const auto& binding : GetBindings(current))
      {
        for (unsigned int vv = 0; vv < targets.GetNumberOfElements(); ++vv)
        {
          auto* view = vtkSMViewProxy::SafeDownCast(targets.GetAsProxy(vv));
          vtkSMSceneGraphNodeProxy* holder = nullptr;
          auto* other =
            view ? this->FindOtherBinding(binding.first, binding.second, view, holder) : nullptr;
          if (other)
          {
            flag(BindingConflict, 2,
              "Node '" + nameOf(current) + "' is bound to " +
                BindingLabel(pxm, binding.first, binding.second) + ", as is node '" +
                NodeLabel(holder) + "' of " + GraphLabel(pxm, other) +
                ", and both graphs target one view.");
            break;
          }
        }
      }
      return true;
    });

  for (const auto& entry : parentCount)
  {
    if (entry.second > 1)
    {
      flag(NotATree, 6,
        "Node '" + nameOf(entry.first) + "' is listed as a child in " +
          std::to_string(entry.second) + " places. A node must have only one parent.");
    }
  }

  if (parentCount.count(root) != 0)
  {
    flag(NotATree, 6, "The root node is listed as child of another node.");
  }

  this->LastError = worst;
  this->LastErrorMessage = worstMessage;
  return worst == NoError;
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::ResetBinding(vtkSMSourceProxy* source, int port)
{
  if (!source)
  {
    return;
  }

  vtkNew<vtkMatrix4x4> identity;
  double values[16];
  vtkMatrix4x4::DeepCopy(values, identity);

  vtkSMPropertyHelper targets(this, "TargetViews");
  for (unsigned int cc = 0; cc < targets.GetNumberOfElements(); ++cc)
  {
    auto* view = vtkSMViewProxy::SafeDownCast(targets.GetAsProxy(cc));
    if (!view)
    {
      continue;
    }
    vtkSMProxy* rep = view->FindRepresentation(source, port);
    if (!rep || !rep->GetProperty("UserTransform"))
    {
      continue;
    }
    vtkSMPropertyHelper(rep, "UserTransform").Set(values, 16);
    rep->UpdateVTKObjects();
  }
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::ResetBindingsOf(vtkSMSceneGraphNodeProxy* node)
{
  for (const auto& binding : GetBindings(node))
  {
    this->ResetBinding(binding.first, binding.second);
  }
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::ApplyToNode(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* world)
{
  const std::vector<Binding> bindings = GetBindings(node);
  if (bindings.empty())
  {
    return;
  }

  double desired[16];
  vtkMatrix4x4::DeepCopy(desired, world);

  vtkSMPropertyHelper targets(this, "TargetViews");
  for (unsigned int cc = 0; cc < targets.GetNumberOfElements(); ++cc)
  {
    auto* view = vtkSMViewProxy::SafeDownCast(targets.GetAsProxy(cc));
    if (!view)
    {
      continue;
    }

    for (const auto& binding : bindings)
    {
      // Look up the source and port in this view to find the specific repr
      vtkSMProxy* rep = view->FindRepresentation(binding.first, binding.second);
      if (!rep || !rep->GetProperty("UserTransform"))
      {
        continue;
      }

      // Skip setting the property and calling UpdateVTKObjects if the desired
      // value is already present.
      vtkSMPropertyHelper transform(rep, "UserTransform");
      double current[16];
      transform.Get(current, 16);
      if (std::equal(current, current + 16, desired))
      {
        continue;
      }

      transform.Set(desired, 16);
      rep->UpdateVTKObjects();
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::TraverseAndApply()
{
  this->ClearError();
  vtkSMSceneGraphNodeProxy* root = this->GetRoot();
  if (!root)
  {
    return;
  }

  // This traversal can't use VisitSubtree() since what is being propagated
  // down the tree (composed transform) is part of the walk, not something
  // a visitor could supply.
  vtkNew<vtkMatrix4x4> identity;
  std::vector<std::pair<vtkSMSceneGraphNodeProxy*, vtkSmartPointer<vtkMatrix4x4>>> pending;
  pending.emplace_back(root, identity.GetPointer());
  std::set<vtkSMSceneGraphNodeProxy*> seen;
  vtkSMSceneGraphNodeProxy* repeated = nullptr;

  while (!pending.empty())
  {
    vtkSMSceneGraphNodeProxy* node = pending.back().first;
    vtkSmartPointer<vtkMatrix4x4> parentWorld = pending.back().second;
    pending.pop_back();

    if (!seen.insert(node).second)
    {
      if (!repeated)
      {
        repeated = node;
      }
      continue;
    }

    vtkNew<vtkMatrix4x4> local;
    ReadLocalMatrix(node, local);

    auto world = vtkSmartPointer<vtkMatrix4x4>::New();
    ComposeWorld(parentWorld, local, world);

    this->ApplyToNode(node, world);

    const std::vector<vtkSMSceneGraphNodeProxy*> children = GetChildVector(node);
    for (auto iter = children.rbegin(); iter != children.rend(); ++iter)
    {
      pending.emplace_back(*iter, world);
    }
  }

  if (repeated)
  {
    this->Fail(NotATree, NotATreeMessage(repeated));
  }
}

//----------------------------------------------------------------------------
void vtkSMSceneGraphProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastError: " << this->LastError << endl;
  os << indent << "LastErrorMessage: " << this->LastErrorMessage << endl;
}
