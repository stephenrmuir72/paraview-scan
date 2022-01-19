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
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMProxyProperty);
//----------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty() = default;

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
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
