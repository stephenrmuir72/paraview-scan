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

#include "vtkObjectFactory.h"
#include "vtkSMPropertyProducer.h"
#include "vtkSMProxyManager.h"

class vtkSMProxyProperty::vtkInternals
{
  vtkSMPropertyProducer ProducerHelper;

public:
  void UpdateProducers(vtkSMProperty* self, const std::vector<vtkSmartPointer<vtkSMProxy>>& inputs)
  {
    std::vector<vtkSMProxy*> proxies;
    std::copy(inputs.begin(), inputs.end(), std::back_inserter(proxies));
    this->ProducerHelper.UpdateProducers(self, proxies);
  }
};

vtkStandardNewMacro(vtkSMProxyProperty);
//----------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
  : Internals(new vtkSMProxyProperty::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty() = default;

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSMProxy> vtkSMProxyProperty::ToValue(const std::string& valueString) const
{
  return nullptr;
}

//----------------------------------------------------------------------------
vtkJson vtkSMProxyProperty::ToJson(const vtkSmartPointer<vtkSMProxy>& value) const
{
  return vtkJson(value ? value->GetGlobalID() : 0);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSMProxy> vtkSMProxyProperty::ToValue(
  const vtkJson& json, vtkSMStateContext*) const
{
  auto* pxm = this->GetProxyManager();
  return pxm ? pxm->FindProxy(json.get<vtkTypeUInt32>()) : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMProxyProperty::Modified()
{
  auto& internals = (*this->Internals);
  internals.UpdateProducers(this, this->GetElements());
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
bool vtkSMProxyProperty::RemoveProxy(vtkSMProxy* proxy)
{
  auto elements = this->GetElements();
  auto iter = std::remove(elements.begin(), elements.end(), proxy);
  elements.erase(iter, elements.end());
  return this->SetElements(elements);
}

//----------------------------------------------------------------------------
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
