/*=========================================================================

  Program:   ParaView
  Module:    vtkProxyDefinitionManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProxyDefinitionManager.h"

#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLParser.h"
#include "vtkSMProxyDefinitionsIterator.h"
#include "vtkSmartPointer.h"

#include <map>
#include <mutex>
#include <thread>

class vtkProxyDefinitionManager::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  std::mutex Mutex;
  std::map<std::string, std::map<std::string, std::shared_ptr<pugi::xml_document>>> Definitions;

  std::shared_ptr<pugi::xml_document> Find(const std::string& group, const std::string& name) const
  {
    auto it1 = this->Definitions.find(group);
    if (it1 != this->Definitions.end())
    {
      auto it2 = it1->second.find(name);
      if (it2 != it1->second.end())
      {
        return it2->second;
      }
    }
    return nullptr;
  }

  void AddDefinition(
    const std::string& group, const std::string& name, const pugi::xml_node& element)
  {
    if (strcmp(element.name(), "Extension") == 0)
    {
      auto coreXML = this->Find(group, name);
      if (!coreXML)
      {
        vtkLogF(ERROR, "Extension specified for unknown proxy type (%s, %s). Skipping",
          group.c_str(), name.c_str());
        return;
      }

      // we cannot modify the XML directly for thread safety. we need to create
      // a clone.

      std::shared_ptr<pugi::xml_document> doc = std::make_shared<pugi::xml_document>();
      auto clone = doc->append_copy(*coreXML);
      for (auto child : element.children())
      {
        clone.append_copy(child);
      }
      // replace previous definition
      this->Definitions[group][name] = doc;
    }
    else
    {
      std::shared_ptr<pugi::xml_document> doc = std::make_shared<pugi::xml_document>();
      doc->append_copy(element);
      this->Definitions[group][name] = doc;
    }
  }
};

vtkStandardNewMacro(vtkProxyDefinitionManager);
//----------------------------------------------------------------------------
vtkProxyDefinitionManager::vtkProxyDefinitionManager()
  : Internals(new vtkProxyDefinitionManager::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkProxyDefinitionManager::~vtkProxyDefinitionManager() = default;

//----------------------------------------------------------------------------
std::shared_ptr<pugi::xml_document> vtkProxyDefinitionManager::FindProxy(
  const std::string& group, const std::string& name) const
{
  auto& internals = (*this->Internals);
  std::lock_guard<std::mutex> lk(internals.Mutex);

  auto it1 = internals.Definitions.find(group);
  if (it1 != internals.Definitions.end())
  {
    auto it2 = it1->second.find(name);
    if (it2 != it1->second.end())
    {
      return it2->second;
    }
  }

  vtkLogF(TRACE, "Failed to find proxy definition (%s, %s)", group.c_str(), name.c_str());
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkProxyDefinitionManager::LoadConfigurationXML(const pugi::xml_node& root)
{
  if (!root)
  {
    return false;
  }
  if (!root.name() || strcmp(root.name(), "ServerManagerConfiguration") != 0)
  {
    // find nested ServerManagerConfiguration element and process that.
    return this->LoadConfigurationXML(root.child("ServerManagerConfiguration"));
  }

  auto& internals = (*this->Internals);
  std::unique_lock<std::mutex> lk(internals.Mutex);

  // Loop over the top-level elements: `ProxyGroup`
  for (pugi::xml_node group : root.children("ProxyGroup"))
  {
    const std::string groupName = group.attribute("name").as_string();

    // Loop over the top-level elements.
    for (pugi::xml_node proxy : group.children())
    {
      const std::string proxyName = proxy.attribute("name").as_string();
      if (!proxyName.empty())
      {
        internals.AddDefinition(groupName, proxyName, proxy);
      }
    }
  }

  lk.unlock(); // must unlock before firing the event.

  this->InvokeEvent(vtkProxyDefinitionManager::ProxyDefinitionsUpdated);
  return true;
}

//----------------------------------------------------------------------------
bool vtkProxyDefinitionManager::LoadConfigurationXML(const std::string& xmlcontents)
{
  pugi::xml_document doc;
  bool success = doc.load_buffer(xmlcontents.c_str(), xmlcontents.length());
  return success ? this->LoadConfigurationXML(doc) : false;
}

//----------------------------------------------------------------------------
vtkSMProxyDefinitionsRange vtkProxyDefinitionManager::GetDefinitions(const std::string& groupName)
{
  auto& internals = (*this->Internals);
  return vtkSMProxyDefinitionsRange(internals.Definitions, groupName);
}

//----------------------------------------------------------------------------
void vtkProxyDefinitionManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
