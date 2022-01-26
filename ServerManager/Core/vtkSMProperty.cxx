/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProperty.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"
#include "vtkStringUtilities.h"
#include "vtkWeakPointer.h"

#include <vtksys/SystemTools.hxx>

class vtkSMProperty::vtkInternals
{
public:
  vtkWeakPointer<vtkSMProxy> ParentProxy;
  vtkWeakPointer<vtkSMProperty> InformationProperty;
  std::string Name;
  std::string Label;
  std::string InformationPropertyName;
  std::string PanelVisibility{ "default" };
  std::string PanelVisibilityDefaultForRepresentation;
  std::string PanelWidget;
  bool IsAnimateable{ false };
  bool IsInformationOnly{ false };
  bool IsResizable{ false };
  bool IsInternal{ false };
  bool NoCustomDefault{ false };
  bool DisableSubTrace{ false };
  size_t NumberOfComponents{ 1 };

  std::vector<vtkSmartPointer<vtkSMDomain>> Domains;
  std::vector<vtkWeakPointer<vtkSMDomain>> DependentDomains;
};

//----------------------------------------------------------------------------
vtkSMProperty::vtkSMProperty()
  : Internals(new vtkSMProperty::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMProperty::~vtkSMProperty() = default;

//----------------------------------------------------------------------------
const std::string& vtkSMProperty::GetName() const
{
  const auto& internals = (*this->Internals);
  return internals.Name;
}

//----------------------------------------------------------------------------
std::string vtkSMProperty::GetLabel() const
{
  const auto& internals = (*this->Internals);
  return internals.Label.empty() ? vtkStringUtilities::GetPrettyLabel(internals.Name)
                                 : internals.Label;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::GetIsAnimateable() const
{
  const auto& internals = (*this->Internals);
  return internals.IsAnimateable;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::GetIsInformationOnly() const
{
  const auto& internals = (*this->Internals);
  return internals.IsInformationOnly;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::GetIsResizable() const
{
  const auto& internals = (*this->Internals);
  return internals.IsResizable;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMProperty::GetInformationProperty() const
{
  const auto& internals = (*this->Internals);
  return internals.InformationProperty;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::GetIsInternal() const
{
  const auto& internals = (*this->Internals);
  return internals.IsInternal;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::GetNoCustomDefault() const
{
  const auto& internals = (*this->Internals);
  return internals.NoCustomDefault;
}

//----------------------------------------------------------------------------
const std::string& vtkSMProperty::GetPanelVisibility() const
{
  const auto& internals = (*this->Internals);
  return internals.PanelVisibility;
}

//----------------------------------------------------------------------------
const std::string& vtkSMProperty::GetPanelVisibilityDefaultForRepresentation() const
{
  const auto& internals = (*this->Internals);
  return internals.PanelVisibilityDefaultForRepresentation;
}

//----------------------------------------------------------------------------
const std::string& vtkSMProperty::GetPanelWidget() const
{
  const auto& internals = (*this->Internals);
  return internals.PanelWidget;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::GetDisableSubTrace() const
{
  const auto& internals = (*this->Internals);
  return internals.DisableSubTrace;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMProperty::GetParentProxy() const
{
  const auto& internals = (*this->Internals);
  return internals.ParentProxy;
}

//----------------------------------------------------------------------------
void vtkSMProperty::SetParentProxy(vtkSMProxy* proxy)
{
  auto& internals = (*this->Internals);
  internals.ParentProxy = proxy;
}

//----------------------------------------------------------------------------
void vtkSMProperty::SetName(const std::string& name)
{
  auto& internals = (*this->Internals);
  internals.Name = name;
}

//----------------------------------------------------------------------------
void vtkSMProperty::SetLabel(const std::string& label)
{
  auto& internals = (*this->Internals);
  internals.Label = label;
}

//----------------------------------------------------------------------------
const std::string& vtkSMProperty::GetInformationPropertyName() const
{
  const auto& internals = (*this->Internals);
  return internals.InformationPropertyName;
}

//----------------------------------------------------------------------------
void vtkSMProperty::SetInformationProperty(vtkSMProperty* property)
{
  auto& internals = (*this->Internals);
  internals.InformationProperty = property;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::ReadXMLAttributes(const pugi::xml_node& node)
{
  auto& internals = (*this->Internals);
  internals.Name = node.attribute("name").value();
  internals.Label = node.attribute("label").value();
  internals.IsAnimateable = node.attribute("animateable").as_bool(false);
  internals.IsInformationOnly = node.attribute("information_only").as_bool(false);
  internals.IsResizable = node.attribute("repeatable").as_bool(false) ||
    node.attribute("repeat_command").as_bool(false) || node.attribute("resizable").as_bool(false);
  internals.IsInternal = node.attribute("is_internal").as_bool(false);
  internals.NoCustomDefault = node.attribute("no_custom_default").as_bool(false);
  internals.DisableSubTrace = node.attribute("disable_sub_trace").as_bool(false);
  internals.InformationPropertyName = node.attribute("information_property").value();
  internals.PanelVisibility =
    node.attribute("panel_visibility")
      .as_string(internals.IsInternal || internals.IsInformationOnly ? "never" : "default");
  internals.PanelVisibilityDefaultForRepresentation =
    node.attribute("panel_visibility_default_for_representation").value();
  internals.PanelWidget = node.attribute("panel_widget").value();
  internals.NumberOfComponents =
    static_cast<size_t>(node.attribute("number_of_components").as_uint(1));
  return true;
}

//----------------------------------------------------------------------------
vtkJson vtkSMProperty::SaveState(bool verbose) const
{
  vtkJson node;
  if (verbose)
  {
    const auto& internals = (*this->Internals);
    vtkJson domains = vtkJson::array();
    for (auto& domain : internals.Domains)
    {
      auto domainState = domain->SaveState(verbose);
      if (!domainState.is_null())
      {
        domains.push_back(std::move(domainState));
      }
    }
    if (!domains.empty())
    {
      node["domains"] = std::move(domains);
    }
  }
  return node;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::LoadState(const vtkJson& json, vtkSMStateContext* context)
{
  // TODO: ASYNC, if domain state is present, give the domains a chance to load
  // the states.
  return true;
}

//----------------------------------------------------------------------------
void vtkSMProperty::AddDomain(vtkSMDomain* domain)
{
  auto& internals = (*this->Internals);
  internals.Domains.push_back(domain);
  domain->AddObserver(vtkCommand::ModifiedEvent, this, &vtkSMProperty::HandleDomainModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMProperty::HandleDomainModifiedEvent()
{
  this->InvokeEvent(vtkCommand::DomainModifiedEvent);
}

//----------------------------------------------------------------------------
size_t vtkSMProperty::GetNumberOfElements() const
{
  return 0;
}

//----------------------------------------------------------------------------
size_t vtkSMProperty::GetNumberOfUncheckedElements() const
{
  return 0;
}

//----------------------------------------------------------------------------
size_t vtkSMProperty::GetNumberOfComponents() const
{
  const auto& internals = (*this->Internals);
  return internals.IsResizable ? internals.NumberOfComponents : this->GetNumberOfElements();
}

//----------------------------------------------------------------------------
vtkClientSession* vtkSMProperty::GetSession() const
{
  auto* proxy = this->GetParentProxy();
  return proxy ? proxy->GetSession() : nullptr;
}

//----------------------------------------------------------------------------
vtkSMProxyManager* vtkSMProperty::GetProxyManager() const
{
  auto* proxy = this->GetParentProxy();
  return proxy ? proxy->GetProxyManager() : nullptr;
}

//----------------------------------------------------------------------------
int vtkSMProperty::GetPropertyType(const std::string& className)
{
  if (className == "vtkSMIntVectorProperty")
  {
    return INT;
  }
  if (className == "vtkSMDoubleVectorProperty")
  {
    return DOUBLE;
  }
  if (className == "vtkSMIdTypeVectorProperty")
  {
    return ID_TYPE;
  }
  if (className == "vtkSMStringVectorProperty")
  {
    return STRING;
  }
  if (className == "vtkSMProxyProperty")
  {
    return PROXY;
  }
  if (className == "vtkSMInputProperty")
  {
    return INPUT;
  }

  return INVALID;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::ResetToDefaults(bool useUnchecked)
{
  return this->ResetToDomainDefaults(useUnchecked) ? true : this->ResetToXMLDefaults(useUnchecked);
}

//----------------------------------------------------------------------------
bool vtkSMProperty::ResetToXMLDefaults(bool vtkNotUsed(useUnchecked))
{
  if (const auto* infoProperty = this->GetInformationProperty())
  {
    this->Copy(infoProperty);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMProperty::ResetToDomainDefaults(bool useUnchecked)
{
  const auto& internals = (*this->Internals);
  for (auto& domain : internals.Domains)
  {
    if (domain->SetDefaultValues(this, useUnchecked))
    {
      vtkLogF(INFO, "Property (%s) reset to default using domain (%s)", this->GetName().c_str(),
        domain->GetName().c_str());
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMProperty::AddDependentDomain(vtkSMDomain* domain)
{
  auto& internals = (*this->Internals);
  internals.DependentDomains.push_back(domain);
}

//----------------------------------------------------------------------------
void vtkSMProperty::UncheckedPropertyModified()
{
  this->UpdateDependentDomains();
  this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMProperty::UpdateDependentDomains() const
{
  vtkLogF(INFO, "UpdateDependentDomains");
  const auto& internals = (*this->Internals);
  for (auto& domain : internals.DependentDomains)
  {
    if (domain)
    {
      domain->Update();
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
