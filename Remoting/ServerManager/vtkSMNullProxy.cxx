/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNullProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNullProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMNullProxy);
//-----------------------------------------------------------------------------
vtkSMNullProxy::vtkSMNullProxy() = default;

//-----------------------------------------------------------------------------
vtkSMNullProxy::~vtkSMNullProxy() = default;

//-----------------------------------------------------------------------------
int vtkSMNullProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  auto retVal = this->Superclass::ReadXMLAttributes(pm, element);
  this->SetVTKClassName(nullptr);
  return retVal;
}

//-----------------------------------------------------------------------------
void vtkSMNullProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
