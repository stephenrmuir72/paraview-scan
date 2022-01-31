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
#include "vtkPVXMLElement.h"
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
  std::map<std::string, std::map<std::string, vtkSmartPointer<vtkPVXMLElement>>> Definitions;

  vtkSmartPointer<vtkPVXMLElement> Find(const std::string& group, const std::string& name) const
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

  void AddDefinition(const std::string& group, const std::string& name, vtkPVXMLElement* element)
  {
    if (strcmp(element->GetName(), "Extension") == 0)
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
      auto clone = vtkInternals::Clone(coreXML);
      for (unsigned int cc = 0; cc < element->GetNumberOfNestedElements(); ++cc)
      {
        clone->AddNestedElement(element->GetNestedElement(cc));
      }
    }
    else
    {
      this->Definitions[group][name] = element;
    }
  }

private:
  vtkSmartPointer<vtkPVXMLElement> Clone(vtkPVXMLElement* source)
  {
    auto clone = vtk::TakeSmartPointer(vtkPVXMLElement::New());
    source->CopyTo(clone);
    return clone;
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
vtkSmartPointer<vtkPVXMLElement> vtkProxyDefinitionManager::FindProxy(
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
bool vtkProxyDefinitionManager::LoadConfigurationXML(vtkPVXMLElement* root)
{
  if (!root)
  {
    return false;
  }
  if (!root->GetName() || strcmp(root->GetName(), "ServerManagerConfiguration") != 0)
  {
    // find nested ServerManagerConfiguration element and process that.
    return this->LoadConfigurationXML(root->FindNestedElementByName("ServerManagerConfiguration"));
  }

  auto& internals = (*this->Internals);
  std::unique_lock<std::mutex> lk(internals.Mutex);

  // Loop over the top-level elements.
  for (unsigned int i = 0; i < root->GetNumberOfNestedElements(); ++i)
  {
    vtkPVXMLElement* group = root->GetNestedElement(i);
    const std::string groupName = group->GetAttributeOrEmpty("name");

    // Loop over the top-level elements.
    for (unsigned int cc = 0; cc < group->GetNumberOfNestedElements(); ++cc)
    {
      vtkPVXMLElement* proxy = group->GetNestedElement(cc);
      const std::string proxyName = proxy->GetAttributeOrEmpty("name");
      if (!proxyName.empty())
      {
        internals.AddDefinition(groupName.c_str(), proxyName.c_str(), proxy);
      }
    }
  }

  lk.unlock(); // must unlock before firing the event.

  this->InvokeEvent(vtkProxyDefinitionManager::ProxyDefinitionsUpdated);
  return true;
}

//----------------------------------------------------------------------------
bool vtkProxyDefinitionManager::LoadConfigurationXML(const char* xmlcontents)
{
  vtkNew<vtkPVXMLParser> parser;
  auto root = parser->ParseXML(xmlcontents);
  return root ? this->LoadConfigurationXML(root) : false;
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
