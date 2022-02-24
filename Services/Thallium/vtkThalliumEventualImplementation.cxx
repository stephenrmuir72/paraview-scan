/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumEventualImplementation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThalliumEventualImplementation.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkThalliumEventualImplementation::vtkThalliumEventualImplementation()
  : vtkEventualImplementation(vtkNJson{})
{
}

//----------------------------------------------------------------------------
bool vtkThalliumEventualImplementation::Test() const
{
  return this->Eventual.test();
}

//----------------------------------------------------------------------------
vtkNJson vtkThalliumEventualImplementation::Wait() const
{
  return this->Eventual.wait();
}

//----------------------------------------------------------------------------
void vtkThalliumEventualImplementation::SetValue(const vtkNJson& value) const
{
  this->Eventual.set_value(value);
  this->vtkEventualImplementation::SetValue(value);
}
