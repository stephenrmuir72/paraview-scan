/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNamedIntVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNamedIntVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMStateLocator.h"
#include "vtkSMVectorPropertyTemplate.h"

vtkStandardNewMacro(vtkSMNamedIntVectorProperty);

vtkSMNamedIntVectorProperty::vtkSMNamedIntVectorProperty()
{
  this->PropertyName = nullptr;
}

vtkSMNamedIntVectorProperty::~vtkSMNamedIntVectorProperty()
{
  this->SetPropertyName(nullptr);
}

void vtkSMNamedIntVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PropertyName: " << (this->PropertyName ? this->PropertyName : "(null)") << "\n";
}

int vtkSMNamedIntVectorProperty::ReadXMLAttributes(vtkSMProxy* proxy, vtkPVXMLElement* element)
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

void vtkSMNamedIntVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMNamedIntVectorProperty* dsrc = vtkSMNamedIntVectorProperty::SafeDownCast(src);
  if (dsrc)
  {
    this->SetPropertyName(dsrc->GetPropertyName());
  }
}
