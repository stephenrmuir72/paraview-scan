/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyPropertyInternals.h"

#include "vtkCommand.h"
#include "vtkJson.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <cassert>

namespace
{
template <class T>
void vtkGetProxyListValues(
  T& tvalues, vtkSMProxyListDomain* tpld, const T& svalues, vtkSMProxyListDomain* spld)
{
  for (typename T::const_iterator siter = svalues.begin(); siter != svalues.end(); ++siter)
  {
    vtkSMProxy* svalue = siter->GetPointer();
    vtkSMProxy* tvalue = tpld->GetProxyWithName(spld->GetProxyName(svalue));
    if (tvalue && svalue)
    {
      tvalue->Copy(svalue);
    }
    tvalues.push_back(tvalue);
  }
}
}

bool vtkSMProxyProperty::CreateProxyAllowed = false; // static init

//***************************************************************************
vtkStandardNewMacro(vtkSMProxyProperty);
//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->PPInternals = new vtkSMProxyProperty::vtkPPInternals(this);
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  delete this->PPInternals;
  this->PPInternals = nullptr;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy)
{
  if (this->PPInternals->Add(proxy))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveProxy(vtkSMProxy* proxy)
{
  if (this->PPInternals->Remove(proxy))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllProxies()
{
  if (this->PPInternals->Clear())
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetProxy(unsigned int index, vtkSMProxy* proxy)
{
  if (this->PPInternals->Set(index, proxy))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetProxies(unsigned int numProxies, vtkSMProxy* proxies[])
{
  if (this->PPInternals->Set(numProxies, proxies))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetNumberOfProxies(unsigned int count)
{
  if (this->PPInternals->Resize(count))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddUncheckedProxy(vtkSMProxy* proxy)
{
  if (this->PPInternals->AddUnchecked(proxy))
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy)
{
  if (this->PPInternals->SetUnchecked(idx, proxy))
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllUncheckedProxies()
{
  if (this->PPInternals->ClearUnchecked())
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
bool vtkSMProxyProperty::IsProxyAdded(vtkSMProxy* proxy)
{
  return this->PPInternals->IsAdded(proxy);
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfProxies()
{
  return static_cast<unsigned int>(this->PPInternals->GetProxies().size());
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfUncheckedProxies()
{
  if (!this->PPInternals->GetUncheckedProxies().empty())
  {
    return static_cast<unsigned int>(this->PPInternals->GetUncheckedProxies().size());
  }
  else
  {
    return this->GetNumberOfProxies();
  }
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetProxy(unsigned int idx)
{
  return this->PPInternals->Get(idx);
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetUncheckedProxy(unsigned int idx)
{
  return this->PPInternals->GetUnchecked(idx);
}
//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetNumberOfUncheckedProxies(unsigned int count)
{
  if (this->PPInternals->ResizeUnchecked(count))
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element)
{
  return this->Superclass::ReadXMLAttributes(parent, element);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);
  vtkSMProxyProperty* psrc = vtkSMProxyProperty::SafeDownCast(src);
  if (!psrc)
  {
    return;
  }

  auto spld = src->FindDomain<vtkSMProxyListDomain>();
  auto tpld = this->FindDomain<vtkSMProxyListDomain>();

  bool modified = false;
  bool unchecked_modified = false;

  if (spld && tpld && psrc->GetNumberOfProxies() > 0)
  {
    vtkPPInternals::SmartVectorOfProxies tvalues;
    vtkGetProxyListValues(tvalues, tpld, psrc->PPInternals->GetProxies(), spld);
    modified = this->PPInternals->SetProxies(tvalues, psrc->PPInternals->GetPorts());

    vtkPPInternals::WeakVectorOfProxies tuvalues;
    vtkGetProxyListValues(tuvalues, tpld, psrc->PPInternals->GetUncheckedProxies(), spld);
    unchecked_modified =
      this->PPInternals->SetUncheckedProxies(tuvalues, psrc->PPInternals->GetUncheckedPorts());
  }
  else
  {
    modified =
      this->PPInternals->SetProxies(psrc->PPInternals->GetProxies(), psrc->PPInternals->GetPorts());
    unchecked_modified = this->PPInternals->SetUncheckedProxies(
      psrc->PPInternals->GetUncheckedProxies(), psrc->PPInternals->GetUncheckedPorts());
  }

  if (modified)
  {
    this->Modified();
  }
  if (modified || unchecked_modified)
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // os << indent << "Values: ";
  // for (unsigned int i=0; i<this->GetNumberOfProxies(); i++)
  //  {
  //  os << this->GetProxy(i) << " ";
  //  }
  // os << endl;
}
//---------------------------------------------------------------------------
void vtkSMProxyProperty::SaveStateValues(vtkPVXMLElement* propertyElement)
{
  unsigned int size = this->GetNumberOfProxies();
  if (size > 0)
  {
    propertyElement->AddAttribute("number_of_elements", size);
  }
  for (unsigned int i = 0; i < size; i++)
  {
    this->AddProxyElementState(propertyElement, i);
  }
}
//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyProperty::AddProxyElementState(vtkPVXMLElement* prop, unsigned int idx)
{
  vtkSMProxy* proxy = this->GetProxy(idx);
  vtkPVXMLElement* proxyElement = nullptr;
  if (proxy)
  {
    proxyElement = vtkPVXMLElement::New();
    proxyElement->SetName("Proxy");
    proxyElement->AddAttribute("value", static_cast<unsigned int>(proxy->GetGlobalID()));
    prop->AddNestedElement(proxyElement);
    proxyElement->FastDelete();
  }
  return proxyElement;
}
//---------------------------------------------------------------------------
// NOTE: This method is duplicated in some way in vtkSMInputProperty
// Therefore, care must be taken to keep the two in sync.
int vtkSMProxyProperty::LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader)
{
  if (!loader)
  {
    // no loader specified, state remains unchanged.
    return 1;
  }

  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  this->Superclass::LoadState(element, loader);

  // If "clear" is present and is 0, it implies that the proxy elements
  // currently in the property should not be cleared before loading
  // the new state.
  int clear = 1;
  element->GetScalarAttribute("clear", &clear);
  if (clear)
  {
    this->PPInternals->Clear();
  }

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() &&
      (strcmp(currentElement->GetName(), "Element") == 0 ||
        strcmp(currentElement->GetName(), "Proxy") == 0))
    {
      int id;
      if (currentElement->GetScalarAttribute("value", &id))
      {
        if (id)
        {
          int port = 0;
          // if output_port is not present, port remains 0.
          currentElement->GetScalarAttribute("output_port", &port);
          vtkSMProxy* proxy = loader->LocateProxy(id);
          if (proxy)
          {
            this->PPInternals->Add(proxy, port);
          }
          else
          {
            // It is not an error to have missing proxies in a proxy property.
            // We simply ignore such proxies.
            // vtkErrorMacro("Could not create proxy of id: " << id);
            // return 0;
          }
        }
        else
        {
          this->PPInternals->Add(nullptr);
        }
      }
    }
  }

  // Do not immediately update. Leave it to the loader.
  this->Modified();
  this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);

  this->ImmediateUpdate = prevImUpdate;
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::EnableProxyCreation()
{
  vtkSMProxyProperty::CreateProxyAllowed = true;
}
//---------------------------------------------------------------------------
void vtkSMProxyProperty::DisableProxyCreation()
{
  vtkSMProxyProperty::CreateProxyAllowed = false;
}
//---------------------------------------------------------------------------
bool vtkSMProxyProperty::CanCreateProxy()
{
  return vtkSMProxyProperty::CreateProxyAllowed;
}

//---------------------------------------------------------------------------
bool vtkSMProxyProperty::IsValueDefault()
{
  // proxy properties are default only if they contain no proxies
  return this->GetNumberOfProxies() == 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::ResetToXMLDefaults()
{
  this->RemoveAllProxies();
}

//---------------------------------------------------------------------------
bool vtkSMProxyProperty::SaveState(vtkJson& element)
{
  this->vtkSMProperty::SaveState(element);
  auto& internals = (*this->PPInternals);
  element["type"] = this->GetElementType(); // variant->set_type(Variant::INPUT); // or PROXY?
  auto array = vtkJson::array();
  for (auto proxy : internals.GetProxies())
  {
    array.push_back(this->ToJson(proxy));
  }
  element["elements"] = std::move(array);

  return true;
}

//---------------------------------------------------------------------------
vtkJson vtkSMProxyProperty::ToJson(const vtkSmartPointer<vtkSMProxy>& value) const
{
  return vtkJson(value ? value->GetGlobalID() : 0);
}
