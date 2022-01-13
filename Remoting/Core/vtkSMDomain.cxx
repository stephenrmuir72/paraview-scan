/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDomain.h"

#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h"

#include <cassert>
#include <map>

class vtkSMDomain::vtkInternals
{
public:
  vtkWeakPointer<vtkSMProperty> Property;
  std::map<std::string, vtkWeakPointer<vtkSMProperty>> RequiredProperties;

  std::string Name;
  bool IsOptional{ false };
};

//---------------------------------------------------------------------------
vtkSMDomain::vtkSMDomain()
  : Internals(new vtkSMDomain::vtkInternals())
{
}

//---------------------------------------------------------------------------
vtkSMDomain::~vtkSMDomain() = default;

//---------------------------------------------------------------------------
const std::string& vtkSMDomain::GetName() const
{
  const auto& internals = (*this->Internals);
  return internals.Name;
}

//---------------------------------------------------------------------------
bool vtkSMDomain::GetIsOptional() const
{
  const auto& internals = (*this->Internals);
  return internals.IsOptional;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMDomain::GetProperty() const
{
  const auto& internals = (*this->Internals);
  return internals.Property;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMDomain::GetRequiredProperty(const std::string& function) const
{
  const auto& internals = (*this->Internals);
  auto iter = internals.RequiredProperties.find(function);
  return iter != internals.RequiredProperties.end() ? iter->second.GetPointer() : nullptr;
}

//---------------------------------------------------------------------------
unsigned int vtkSMDomain::GetNumberOfRequiredProperties() const
{
  const auto& internals = (*this->Internals);
  return static_cast<unsigned int>(internals.RequiredProperties.size());
}

//---------------------------------------------------------------------------
void vtkSMDomain::SetProperty(vtkSMProperty* property)
{
  auto& internals = (*this->Internals);
  internals.Property = property;
}

//---------------------------------------------------------------------------
bool vtkSMDomain::ReadXMLAttributes(const pugi::xml_node& node)
{
  auto* property = this->GetProperty();
  assert(property != nullptr);

  auto* proxy = property->GetParentProxy();

  auto& internals = (*this->Internals);
  internals.IsOptional = node.attribute("optional").as_bool(false);

  std::set<vtkSMProperty*> propertiesSet;
  for (auto child : node.children("RequiredProperties"))
  {
    const std::string pname = child.attribute("name").as_string();
    const std::string function = child.attribute("function").as_string();
    if (function.empty())
    {
      vtkLogF(ERROR, "'function' must be specified for a required property.");
      continue;
    }

    if (auto* requiredProperty = proxy->GetProperty(pname))
    {
      internals.RequiredProperties[function] = requiredProperty;
      propertiesSet.insert(requiredProperty);
    }
  }

  // Update the domain if any of the properties have their unchecked values
  // modified. This could be setup to use the session event loop to sqaush
  // multiple invocation --- something to look into in the future.
  for (auto& aProperty : propertiesSet)
  {
    aProperty->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, this, &vtkSMDomain::Update);
  }

  return true;
}

//---------------------------------------------------------------------------
vtkJson vtkSMDomain::SaveState(bool vtkNotUsed(verbose)) const
{
  return vtkJson();
}

//---------------------------------------------------------------------------
void vtkSMDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
