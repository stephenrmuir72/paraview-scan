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

#include "vtkObjectFactory.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMInputProperty);
//----------------------------------------------------------------------------
vtkSMInputProperty::vtkSMInputProperty() = default;

//----------------------------------------------------------------------------
vtkSMInputProperty::~vtkSMInputProperty() = default;

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSMOutputPort> vtkSMInputProperty::ToValue(const std::string& valueString) const
{
  return nullptr;
}

//----------------------------------------------------------------------------
vtkJson vtkSMInputProperty::ToJson(const vtkSmartPointer<vtkSMOutputPort>& value) const
{
  vtkJson json = vtkJson::object();
  json["producer"] = value ? value->GetProducer()->GetGlobalID() : 0;
  json["index"] = value ? value->GetIndex() : 0;
  return json;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSMOutputPort> vtkSMInputProperty::ToValue(
  const vtkJson& json, vtkSMStateContext*) const
{
  auto* pxm = this->GetProxyManager();
  auto producer = pxm ? pxm->FindProxy(json.at("producer").get<vtkTypeUInt32>()) : nullptr;
  auto index = producer ? json.at("index").get<int>() : 0;
  return producer ? producer->GetOutputPort(index) : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
