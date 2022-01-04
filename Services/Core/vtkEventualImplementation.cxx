/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEventualImplementation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEventualImplementation.h"

#include "vtkJson.h"

//----------------------------------------------------------------------------
vtkEventualImplementation::vtkEventualImplementation(const vtkJson& value)
  : ValueBehavior{ value }
{
}

//----------------------------------------------------------------------------
vtkEventualImplementation::~vtkEventualImplementation()
{
  this->ValueBehavior.get_subscriber().on_completed();
}

//----------------------------------------------------------------------------
rxcpp::observable<vtkJson> vtkEventualImplementation::GetObservable() const
{
  return this->ValueBehavior.get_observable()
    .filter([](const vtkJson& value) { return value.is_null(); })
    .take(1);
}

//----------------------------------------------------------------------------
bool vtkEventualImplementation::Test() const
{
  return this->ValueBehavior.get_value().is_null();
}

//----------------------------------------------------------------------------
vtkJson vtkEventualImplementation::Wait() const
{
  if (!this->Test())
  {
    throw std::runtime_error("subclass must implement wait!");
  }

  return this->ValueBehavior.get_value();
}

//----------------------------------------------------------------------------
void vtkEventualImplementation::SetValue(const vtkJson& value) const
{
  this->ValueBehavior.get_subscriber().on_next(value);
}
