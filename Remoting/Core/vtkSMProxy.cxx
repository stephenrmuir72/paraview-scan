/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxy.h"

#include "vtkClientSession.h"
#include "vtkCommand.h"
#include "vtkGarbageCollector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPacket.h"
#include "vtkProcessModule.h"
#include "vtkProxyDefinitionManager.h"
#include "vtkRemoteObjectProvider.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkSMDomain.h"
#include "vtkSMInstantiator.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkServicesEngine.h"
#include "vtkSmartPointer.h"
#include "vtkStringUtilities.h"
#include "vtkWeakPointer.h"

#include <vtksys/SystemTools.hxx>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

#include <cassert>

class vtkSMProxy::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };

  vtkWeakPointer<vtkClientSession> Session;
  vtkWeakPointer<vtkSMProxyManager> ProxyManager;
  std::vector<vtkSMOutputPort*> OutputPorts;

  vtkTypeUInt32 UpdateInformationTag{ 0 };
  rxcpp::subjects::behavior<std::pair<vtkTypeUInt32, bool>> UpdateInformationSubject{
    std::make_pair(0, false)
  };

  vtkTypeUInt32 UpdatePipelineTag{ 0 };
  rxcpp::subjects::behavior<std::pair<vtkTypeUInt32, bool>> UpdatePipelineSubject{ std::make_pair(
    0, false) };

  vtkTypeUInt32 GlobalID{ 0 };
  vtkTypeUInt32 Location{ vtkClientSession::DATA_SERVER };
  std::string Group;
  std::string Name;
  std::string XMLLabel;
  std::string VTKClassName;
  bool Dirty{ true };

  // This is a map of all public properties on this proxy i.e. properties on
  // this proxy as well as exposed properties from the sub-proxies.
  std::map<std::string, vtkSmartPointer<vtkSMProperty>> Properties;

  // This helps preserve order for properties as they are defined.
  std::vector<std::string> PropertyNamesOrder;

  std::map<std::string, vtkSmartPointer<vtkSMProxy>> SubProxies;

  vtkWeakPointer<vtkSMProperty> AddProperty(const pugi::xml_node& node, vtkSMProxy* self);
  std::vector<vtkWeakPointer<vtkSMProperty>> AddSubProxy(
    const pugi::xml_node& node, vtkSMProxy* self);

  ~vtkInternals()
  {
    for (auto& port : this->OutputPorts)
    {
      if (port)
      {
        port->UnRegister(nullptr);
      }
    }
    this->OutputPorts.clear();
    this->UpdateInformationSubject.get_subscriber().on_completed();
    this->UpdatePipelineSubject.get_subscriber().on_completed();
  }

private:
  bool AddProperty(vtkSMProperty* property, bool overrideIfNeeded)
  {
    const auto& propertyName = property->GetName();
    if (this->Properties.find(propertyName) != this->Properties.end())
    {
      if (!overrideIfNeeded)
      {
        vtkLogF(ERROR, "Duplicate property Property(%s)", propertyName.c_str());
        return false;
      }

      vtkLogF(TRACE, "Duplicate property %s; will override.", propertyName.c_str());
    }
    this->Properties[propertyName] = property;
    return true;
  }
};

//-----------------------------------------------------------------------------
vtkWeakPointer<vtkSMProperty> vtkSMProxy::vtkInternals::AddProperty(
  const pugi::xml_node& node, vtkSMProxy* self)
{
  const auto className = fmt::format("vtkSM{}", node.name());
  auto object = vtkSMInstantiator::CreateInstance(className);
  if (!object)
  {
    vtkLogF(ERROR, "Unknown class type '%s'", className.c_str());
    return nullptr;
  }
  auto* property = vtkSMProperty::SafeDownCast(object);
  if (!property)
  {
    vtkLogF(ERROR, "'%s' must be a 'vtkSMProperty' subclass.", className.c_str());
    return nullptr;
  }

  property->SetParentProxy(self);
  if (!property->ReadXMLAttributes(node))
  {
    vtkLogF(ERROR, "Failed to create property Proxy(%s, %s), Property(%s)", this->Group.c_str(),
      this->Name.c_str(), node.attribute("name").value());
    return nullptr;
  }

  return this->AddProperty(property, node.attribute("override").as_bool(false)) ? property
                                                                                : nullptr;
}

//----------------------------------------------------------------------------
std::vector<vtkWeakPointer<vtkSMProperty>> vtkSMProxy::vtkInternals::AddSubProxy(
  const pugi::xml_node& node, vtkSMProxy* self)
{
  const std::string name = node.name();
  if (name.empty())
  {
    vtkLogF(ERROR, "Cannot add subproxy without name!");
    return {};
  }

  const std::string xmlgroup = node.attribute("proxyname").value();
  const std::string xmlname = node.attribute("proxygroup").value();
  if (xmlgroup.empty() || xmlname.empty())
  {
    vtkLogF(ERROR, "Subproxy '%s' does not specify a type", name.c_str());
    return {};
  }

  auto* pxm = self->GetProxyManager();
  auto subProxy = pxm->NewProxy(xmlgroup, xmlname);
  if (!subProxy)
  {
    return {};
  }

  if (this->SubProxies.find(name) != this->SubProxies.end())
  {
    if (!node.attribute("override").as_bool(false))
    {
      vtkLogF(ERROR, "Subproxy named '%s' already exists.", name.c_str());
      return {};
    }
  }

  this->SubProxies[name] = subProxy;

  std::vector<vtkWeakPointer<vtkSMProperty>> exposedProperties;
  for (auto epXML : node.children("ExposedProperties"))
  {
    for (auto prop : epXML.children("Property"))
    {
      const std::string pname = prop.attribute("name").value();
      const std::string ename = prop.attribute("exposed_name").value();
      const std::string elabel = prop.attribute("label").value();
      auto* subProperty = subProxy->GetProperty(pname);
      if (!subProperty)
      {
        vtkLogF(ERROR, "No property named '%s' on subproxy ('%s'). Skipping.", pname.c_str(),
          name.c_str());
        continue;
      }

      if (!ename.empty())
      {
        // Since an exposed name is provided, change the name on the property.
        // NOTE: this is little funky, we're changing it's name, but the
        // subproxy will still only return the property when requested with
        // its original name since we don't update that map. I think that is
        // reasonable, but let's keep that in mind in case we run into odd
        // issues.
        subProperty->SetName(ename);
      }

      if (!elabel.empty())
      {
        subProperty->SetLabel(elabel);
      }
      else if (!ename.empty())
      {
        // Since the exposed name was changed, more likely than not the original
        // label, if any doesn't make sense. so change it too.
        subProperty->SetLabel(std::string{});
      }

      if (this->AddProperty(subProperty, prop.attribute("override").as_bool(false)))
      {
        exposedProperties.push_back(subProperty);
      }
    }
  }
  return exposedProperties;
}

//=============================================================================
vtkStandardNewMacro(vtkSMProxy);
//----------------------------------------------------------------------------
vtkSMProxy::vtkSMProxy()
  : Internals(new vtkSMProxy::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMProxy::~vtkSMProxy() = default;

//----------------------------------------------------------------------------
void vtkSMProxy::SetSession(vtkClientSession* session)
{
  auto& internals = (*this->Internals);
  internals.Session = session;
  internals.ProxyManager = session ? session->GetProxyManager() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMProxy::SetGlobalID(vtkTypeUInt32 gid)
{
  auto& internals = (*this->Internals);
  internals.GlobalID = gid;
}

//----------------------------------------------------------------------------
void vtkSMProxy::SetGroup(const std::string& group)
{
  auto& internals = (*this->Internals);
  internals.Group = group;
}

//----------------------------------------------------------------------------
void vtkSMProxy::SetName(const std::string& name)
{
  auto& internals = (*this->Internals);
  internals.Name = name;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMProxy::GetGlobalID() const
{
  const auto& internals = (*this->Internals);
  return internals.GlobalID;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMProxy::GetLocation() const
{
  const auto& internals = (*this->Internals);
  return internals.Location;
}

//----------------------------------------------------------------------------
vtkClientSession* vtkSMProxy::GetSession() const
{
  const auto& internals = (*this->Internals);
  return internals.Session;
}

//----------------------------------------------------------------------------
vtkSMProxyManager* vtkSMProxy::GetProxyManager() const
{
  const auto& internals = (*this->Internals);
  return internals.ProxyManager;
}

//----------------------------------------------------------------------------
const std::string& vtkSMProxy::GetName() const
{
  const auto& internals = (*this->Internals);
  return internals.Name;
}

//----------------------------------------------------------------------------
const std::string& vtkSMProxy::GetGroup() const
{
  const auto& internals = (*this->Internals);
  return internals.Group;
}

//----------------------------------------------------------------------------
std::string vtkSMProxy::GetXMLLabel() const
{
  const auto& internals = (*this->Internals);
  return internals.XMLLabel.empty() ? vtkStringUtilities::GetPrettyLabel(internals.Name)
                                    : internals.XMLLabel;
}

//----------------------------------------------------------------------------
bool vtkSMProxy::ReadXMLAttributes(const pugi::xml_node& node)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.VTKClassName = node.attribute("class").value();
  internals.XMLLabel = node.attribute("label").value();

  const std::string processes = node.attribute("processes").as_string("dataserver");
  if (!processes.empty())
  {
    internals.Location = 0;
    if (processes.find("client") != std::string::npos)
    {
      internals.Location |= vtkClientSession::CLIENT;
    }
    if (processes.find("renderserver") != std::string::npos)
    {
      internals.Location |= vtkClientSession::RENDER_SERVER;
    }
    if (processes.find("dataserver") != std::string::npos)
    {
      internals.Location |= vtkClientSession::DATA_SERVER;
    }
  }

  // create output ports.
  const int numOutputs = node.attribute("number_of_output_ports").as_int(0);
  internals.OutputPorts.resize(numOutputs, nullptr);
  for (int cc = 0; cc < numOutputs; ++cc)
  {
    auto* port = vtkSMOutputPort::New();
    port->SetIndex(cc);
    port->SetProducer(this);
    internals.OutputPorts[cc] = port;
  }

  // create subproxes and properties.
  std::vector<vtkWeakPointer<vtkSMProperty>> properties;
  for (auto child : node.children())
  {
    if (vtkStringUtilities::EndsWith(child.name(), "Property"))
    {
      if (auto property = internals.AddProperty(child, this))
      {
        properties.push_back(property);
      }
    }
    else if (strcmp(child.name(), "SubProxy") == 0)
    {
      auto exposedProperties = internals.AddSubProxy(child, this);
      std::copy(exposedProperties.begin(), exposedProperties.end(), std::back_inserter(properties));
    }
  }

  // now we process domains. This is done after all properties have been
  // processed so that domains don't end up using overridden properties as
  // required properties.
  for (auto pchild : node.children())
  {
    if (!vtkStringUtilities::EndsWith(pchild.name(), "Property"))
    {
      continue;
    }

    auto* property = this->GetProperty(pchild.attribute("name").as_string());
    if (property->GetParentProxy() != this)
    {
      // property have been overridden by a subproxy's exposed property.
      // don't bother changing domains on this one.
      continue;
    }

    if (!property->GetInformationPropertyName().empty())
    {
      property->SetInformationProperty(this->GetProperty(property->GetInformationPropertyName()));
    }

    for (auto dchild : pchild.children())
    {
      if (!vtkStringUtilities::EndsWith(dchild.name(), "Domain"))
      {
        continue;
      }

      auto object = vtkSMInstantiator::CreateInstance(fmt::format("vtkSM{}", dchild.name()));
      if (auto* domain = vtkSMDomain::SafeDownCast(object))
      {
        domain->SetProperty(property);
        if (domain->ReadXMLAttributes(dchild))
        {
          property->AddDomain(domain);
        }
      }
    }
  }

  // build names order list.
  for (const auto& property : properties)
  {
    if (property)
    {
      internals.PropertyNamesOrder.push_back(property->GetName());
    }
  }

  // Alright, at this point, all sub-proxies and properties have been created.
  // Now, create domains etc. We do this after properties have been processed to
  // ensure overrides etc. are resolved before we start setting up relationships
  // between properties.

  // Add observers for all properties (note, this will include self-properties
  // and exposed properties on subproxies too).
  for (auto& pair : internals.Properties)
  {
    pair.second->AddObserver(vtkCommand::ModifiedEvent, this, &vtkSMProxy::HandlePropertyEvents);
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMProxy::UpdateVTKObjects()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  // update state for subproxies first; this ensures that they are created
  // before they are accessed by the parent proxy, if at all.
  const bool subproxyUpdated = std::accumulate(internals.SubProxies.begin(),
    internals.SubProxies.end(), false, [](bool result, const auto& pair) {
      // TODO: ASYNC
      // At some point we want to enable putting all subproxy states
      // in the same message. Principle #1: sending multiple messages is worse than sending
      // large messages.
      return pair.second->UpdateVTKObjects();
    });

  if (internals.Dirty)
  {
    vtkJson state = this->SaveState(/*verbose=*/false);
    if (!state.empty())
    {
      auto packet = vtkRemoteObjectProvider::UpdateState(internals.GlobalID, state);
      internals.Session->SendMessage(internals.Location, std::move(packet));
    }
  }

  if (subproxyUpdated || internals.Dirty)
  {
    internals.Dirty = false;
    this->InvokeEvent(vtkCommand::UpdateEvent, nullptr);
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
rxcpp::observable<bool> vtkSMProxy::UpdateInformation()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  const vtkTypeUInt32 tag = ++internals.UpdateInformationTag;

  auto subscriber = internals.UpdateInformationSubject.get_subscriber();

  auto packet = vtkRemoteObjectProvider::UpdateInformation(this->GetGlobalID());
  auto coordination = this->GetSession()->GetCoordination();
  auto eventual = internals.Session->SendRequest(internals.Location, std::move(packet));
  eventual.GetObservable()
    .observe_on(coordination)
    .subscribe([this, subscriber, tag](const vtkPacket& packet) {
      vtkLogF(INFO, "handle UpdateInformation response");

      const auto mtime = this->GetMTime();
      // TODO: ASYNC
      // this->LoadState(packet.GetJSON());

      const bool propertiesUpdated = mtime != this->GetMTime();
      subscriber.on_next(std::make_pair(tag, false));
    });

  return internals.UpdateInformationSubject.get_observable()
    .filter([tag](const std::pair<vtkTypeUInt32, bool>& pair) { return tag == pair.first; })
    .take(1)
    .map([](const std::pair<vtkTypeUInt32, bool>& pair) { return pair.second; })
    .observe_on(coordination);
}

//----------------------------------------------------------------------------
rxcpp::observable<bool> vtkSMProxy::UpdatePipeline(double time)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  const vtkTypeUInt32 tag = ++internals.UpdatePipelineTag;

  auto subscriber = internals.UpdatePipelineSubject.get_subscriber();
  auto packet = vtkRemoteObjectProvider::UpdatePipeline(this->GetGlobalID(), time);
  auto eventual = internals.Session->SendRequest(internals.Location, std::move(packet));
  eventual.GetObservable().subscribe([this, subscriber, tag](const vtkPacket& reply) {
    vtkLogF(INFO, "handle UpdatePipeline response");
    const bool status = reply.GetJSON().get<bool>();
    subscriber.on_next(std::make_pair(tag, status));
  });

  auto coordination = this->GetSession()->GetCoordination();
  return internals.UpdatePipelineSubject.get_observable()
    .filter([tag](const std::pair<vtkTypeUInt32, bool>& pair) { return tag == pair.first; })
    .take(1)
    .map([](const std::pair<vtkTypeUInt32, bool>& pair) { return pair.second; })
    .observe_on(coordination);
}

//----------------------------------------------------------------------------
int vtkSMProxy::GetNumberOfOutputPorts() const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return static_cast<int>(internals.OutputPorts.size());
}

//----------------------------------------------------------------------------
vtkSMOutputPort* vtkSMProxy::GetOutputPort(int index) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return index >= 0 && index < this->GetNumberOfOutputPorts() ? internals.OutputPorts[index]
                                                              : nullptr;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::GetProperty(const std::string& name) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  auto iter = internals.Properties.find(name);
  return iter != internals.Properties.end() ? iter->second.GetPointer() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMProxy::HandlePropertyEvents(vtkObject* caller, unsigned long eventId, void* callData)
{
  auto* property = vtkSMProperty::SafeDownCast(caller);
  assert(property != nullptr);

  // NOTE: property may be a self-property of an exposed property on a subproxy.

  bool propertiesModified = false;
  auto& internals = (*this->Internals);
  switch (eventId)
  {
    case vtkCommand::ModifiedEvent:
      propertiesModified = true;
      this->InvokeEvent(
        vtkCommand::PropertyModifiedEvent, const_cast<char*>(property->GetName().c_str()));
      break;
  }

  if (propertiesModified)
  {
    // ensures that next `UpdateVTKObjects` call will push state.
    internals.Dirty = true;

    // let the world know the proxy has been modified.
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSMProxy::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  int index = 0;
  for (auto& port : internals.OutputPorts)
  {
    vtkGarbageCollectorReport(collector, port, fmt::format("Port {}", index++).c_str());
  }
}

//----------------------------------------------------------------------------
vtkJson vtkSMProxy::SaveState(bool verbose) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  vtkJson state;
  state["group"] = internals.Group;
  state["name"] = internals.Name;
  state["id"] = internals.GlobalID;
  vtkJson properties = vtkJson::object();
  for (auto& pair : internals.Properties)
  {
    const auto& property = pair.second;
    if (property->GetParentProxy() != this)
    {
      // skip exposed properties from sub proxies; they would have been pushed
      // by the sub proxy itself.
      continue;
    }

    auto propertyState = property->SaveState(verbose);
    if (!propertyState.is_null())
    {
      properties[pair.first] = std::move(propertyState);
    }
  }
  state["properties"] = std::move(properties);
  return state;
}

//----------------------------------------------------------------------------
bool vtkSMProxy::LoadState(const vtkJson& json, vtkSMStateContext* context)
{
  if (json.count("properties") && json["properties"].is_object())
  {
    for (const auto& el : json["properties"].items())
    {
      const auto name = el.key();
      if (auto* property = this->GetProperty(name))
      {
        if (!property->LoadState(el.value(), context))
        {
          return false;
        }
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkPipelineUpdated()
{
  // This is called when the remote VTK object has re-executed.
  vtkLogF(INFO, "Proxy (%s, %s): %u has been updated!", this->GetGroup().c_str(),
    this->GetName().c_str(), this->GetGlobalID());

  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  for (auto& port : internals.OutputPorts)
  {
    // port->InvalidateDataInformation();
  }

  // Notify the world that this algorithm has re-executed.
  this->InvokeEvent(vtkCommand::UpdateDataEvent, nullptr);
}

//----------------------------------------------------------------------------
void vtkSMProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
