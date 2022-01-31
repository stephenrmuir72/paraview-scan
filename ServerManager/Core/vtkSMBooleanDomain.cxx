/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBooleanDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBooleanDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMBooleanDomain);
//---------------------------------------------------------------------------
vtkSMBooleanDomain::vtkSMBooleanDomain() = default;

//---------------------------------------------------------------------------
vtkSMBooleanDomain::~vtkSMBooleanDomain() = default;

//---------------------------------------------------------------------------
int vtkSMBooleanDomain::IsInDomain(vtkSMProperty* property) const
{
  if (this->GetIsOptional())
  {
    return IN_DOMAIN;
  }

  auto iproperty = vtkSMIntVectorProperty::SafeDownCast(property);
  if (!iproperty)
  {
    return NOT_APPLICABLE;
  }

  for (const auto& elem : iproperty->GetElements())
  {
    if (elem != 0 && elem != 1)
    {
      return NOT_IN_DOMAIN;
    }
  }

  return IN_DOMAIN;
}

//---------------------------------------------------------------------------
void vtkSMBooleanDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
