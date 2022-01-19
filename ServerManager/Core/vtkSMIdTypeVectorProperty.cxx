/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIdTypeVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIdTypeVectorProperty.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMIdTypeVectorProperty);
//----------------------------------------------------------------------------
vtkSMIdTypeVectorProperty::vtkSMIdTypeVectorProperty() = default;

//----------------------------------------------------------------------------
vtkSMIdTypeVectorProperty::~vtkSMIdTypeVectorProperty() = default;

//----------------------------------------------------------------------------
vtkIdType vtkSMIdTypeVectorProperty::ToValue(const std::string& valueString) const
{
  return static_cast<vtkIdType>(std::stoll(valueString));
}

//----------------------------------------------------------------------------
vtkJson vtkSMIdTypeVectorProperty::ToJson(const vtkIdType& value) const
{
  return vtkJson(value);
}

//----------------------------------------------------------------------------
vtkIdType vtkSMIdTypeVectorProperty::ToValue(const vtkJson& json, vtkSMStateContext*) const
{
  return json.get<vtkIdType>();
}

//----------------------------------------------------------------------------
void vtkSMIdTypeVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
