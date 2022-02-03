/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputProperty.h"
#include "vtkSMProxyPropertyInternals.h"

#include "vtkCommand.h"
#include "vtkJson.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMSessionProxyManager.h"

vtkStandardNewMacro(vtkSMInputProperty);

//---------------------------------------------------------------------------
vtkSMInputProperty::vtkSMInputProperty()
{
  this->MultipleInput = 0;
  this->PortIndex = 0;
}

//---------------------------------------------------------------------------
vtkSMInputProperty::~vtkSMInputProperty() = default;

//---------------------------------------------------------------------------
int vtkSMInputProperty::ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(parent, element))
  {
    return 0;
  }

  int multiple_input;
  int retVal = element->GetScalarAttribute("multiple_input", &multiple_input);
  if (retVal)
  {
    this->SetMultipleInput(multiple_input);
    this->Repeatable = multiple_input;
  }

  int port_index;
  retVal = element->GetScalarAttribute("port_index", &port_index);
  if (retVal)
  {
    this->SetPortIndex(port_index);
  }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultipleInput: " << this->MultipleInput << endl;
  os << indent << "PortIndex: " << this->PortIndex << endl;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetProxies(
  unsigned int numProxies, vtkSMProxy* proxies[], unsigned int outputports[])
{
  if (this->PPInternals->Set(numProxies, proxies, outputports))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AddInputConnection(vtkSMProxy* proxy, unsigned int outputPort)
{
  if (this->PPInternals->Add(proxy, outputPort))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetInputConnection(
  unsigned int idx, vtkSMProxy* proxy, unsigned int outputPort)
{
  if (this->PPInternals->Set(idx, proxy, outputPort))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AddUncheckedInputConnection(vtkSMProxy* proxy, unsigned int outputPort)
{
  if (this->PPInternals->AddUnchecked(proxy, outputPort))
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetUncheckedInputConnection(
  unsigned int idx, vtkSMProxy* proxy, unsigned int outputPort)
{
  if (this->PPInternals->SetUnchecked(idx, proxy, outputPort))
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetOutputPortForConnection(unsigned int idx)
{
  return this->PPInternals->GetPort(idx);
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetUncheckedOutputPortForConnection(unsigned int idx)
{
  return this->PPInternals->GetUncheckedPort(idx);
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMInputProperty::AddProxyElementState(vtkPVXMLElement* prop, unsigned int idx)
{
  vtkPVXMLElement* proxyElm = this->Superclass::AddProxyElementState(prop, idx);
  if (proxyElm)
  {
    proxyElm->AddAttribute("output_port", this->GetOutputPortForConnection(idx));
  }
  return proxyElm;
}

//----------------------------------------------------------------------------
bool vtkSMInputProperty::SaveState(vtkJson& element)
{
  this->vtkSMProperty::SaveState(element);
  auto& internals = (*this->PPInternals);
  element["type"] = this->GetElementType();
  auto array = vtkJson::array();
  const size_t count = internals.GetProxies().size();
  for (size_t i = 0; i < count; i++)
  {
    vtkJson json = vtkJson::object();
    vtkSMProxy* value = this->GetProxy(i);
    json["producer"] = value ? value->GetGlobalID() : 0;
    json["index"] = value ? internals.GetPort(i) : 0;
    array.push_back(json);
  }
  element["elements"] = std::move(array);

  return true;
}
//---------------------------------------------------------------------------
bool vtkSMInputProperty::LoadState(const vtkJson& json, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::LoadState(json, locator))
  {
    return false;
  }

  if (!json.contains("type") || json.at("type").get<int>() != this->GetElementType())
  {
    vtkLogF(ERROR, "missing 'type' information!");
    return false;
  }

  auto& internals = (*this->PPInternals);
  auto* pxm = this->GetSessionProxyManager();
  if (!pxm)
  {
    vtkLogF(ERROR, "proxy manager is null!");
    return false;
  }
  int i = 0;
  for (const auto& element : json.at("elements"))
  {
    vtkIdType gid = element["producer"].get<vtkTypeUInt32>();
    unsigned int port = element["index"].get<unsigned int>();
    internals.Set(i, pxm->FindProxy(gid), port);
    i++;
  }

  return true;
}
