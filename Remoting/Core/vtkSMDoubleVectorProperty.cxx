/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDoubleVectorProperty.h"

#include "vtkObjectFactory.h"

#include <string>

vtkStandardNewMacro(vtkSMDoubleVectorProperty);
//----------------------------------------------------------------------------
vtkSMDoubleVectorProperty::vtkSMDoubleVectorProperty() = default;

//----------------------------------------------------------------------------
vtkSMDoubleVectorProperty::~vtkSMDoubleVectorProperty() = default;

//----------------------------------------------------------------------------
double vtkSMDoubleVectorProperty::ToValue(const std::string& valueString) const
{
  return std::stod(valueString);
}

//----------------------------------------------------------------------------
vtkJson vtkSMDoubleVectorProperty::ToJson(const double& value) const
{
  return vtkJson(value);
}

//----------------------------------------------------------------------------
double vtkSMDoubleVectorProperty::ToValue(const vtkJson& json, vtkSMStateContext*) const
{
  return json.get<double>();
}

//----------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
