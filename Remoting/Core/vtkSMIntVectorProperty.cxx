/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIntVectorProperty.h"

#include "vtkObjectFactory.h"
#include <string>

vtkStandardNewMacro(vtkSMIntVectorProperty);
//----------------------------------------------------------------------------
vtkSMIntVectorProperty::vtkSMIntVectorProperty() = default;

//----------------------------------------------------------------------------
vtkSMIntVectorProperty::~vtkSMIntVectorProperty() = default;

//----------------------------------------------------------------------------
int vtkSMIntVectorProperty::ToValue(const std::string& valueString) const
{
  return std::stoi(valueString);
}

//----------------------------------------------------------------------------
vtkJson vtkSMIntVectorProperty::ToJson(const int& value) const
{
  return vtkJson(value);
}

//----------------------------------------------------------------------------
int vtkSMIntVectorProperty::ToValue(const vtkJson& json, vtkSMStateContext*) const
{
  return json.get<int>();
}

//----------------------------------------------------------------------------
void vtkSMIntVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
