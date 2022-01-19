/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStringVectorProperty.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMStringVectorProperty);
//----------------------------------------------------------------------------
vtkSMStringVectorProperty::vtkSMStringVectorProperty() = default;

//----------------------------------------------------------------------------
vtkSMStringVectorProperty::~vtkSMStringVectorProperty() = default;

//----------------------------------------------------------------------------
std::string vtkSMStringVectorProperty::ToValue(const std::string& valueString) const
{
  return valueString;
}

//----------------------------------------------------------------------------
vtkJson vtkSMStringVectorProperty::ToJson(const std::string& value) const
{
  return vtkJson(value);
}

//----------------------------------------------------------------------------
std::string vtkSMStringVectorProperty::ToValue(const vtkJson& json, vtkSMStateContext*) const
{
  return json.get<std::string>();
}

//----------------------------------------------------------------------------
void vtkSMStringVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
