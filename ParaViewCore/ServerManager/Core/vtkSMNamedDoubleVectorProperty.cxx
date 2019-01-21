/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNamedDoubleVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNamedDoubleVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMStateLocator.h"
#include "vtkSMVectorPropertyTemplate.h"

vtkStandardNewMacro(vtkSMNamedDoubleVectorProperty);

vtkSMNamedDoubleVectorProperty::vtkSMNamedDoubleVectorProperty()
{
  this->PropertyName = nullptr;
}

vtkSMNamedDoubleVectorProperty::~vtkSMNamedDoubleVectorProperty()
{
  this->SetPropertyName(nullptr);
}

void vtkSMNamedDoubleVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PropertyName: " << (this->PropertyName ? this->PropertyName : "(null)") << "\n";
}

int vtkSMNamedDoubleVectorProperty::ReadXMLAttributes(vtkSMProxy* proxy, vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(proxy, element);
  if (!retVal)
  {
    return retVal;
  }

  const char* propertyName = element->GetAttribute("property_name");
  if (!propertyName)
  {
    retVal = 0;
    vtkErrorMacro("Named vector properties must have a \"property_name\" attribute.");
  }
  else
  {
    this->SetPropertyName(propertyName);
  }
  return retVal;
}

void vtkSMNamedDoubleVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMNamedDoubleVectorProperty* dsrc = vtkSMNamedDoubleVectorProperty::SafeDownCast(src);
  if (dsrc)
  {
    this->SetPropertyName(dsrc->GetPropertyName());
  }
}
