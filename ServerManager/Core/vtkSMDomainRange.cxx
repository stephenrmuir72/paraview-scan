/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMDomainRange.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDomainRange.h"
#include "vtkSMProperty.h"

vtkSMDomainRange::vtkSMDomainRange(vtkSmartPointer<vtkSMProperty> property, VectorType& vector)
  : Vector(vector)
{
  this->Property = property;
}

//------------------------------------------------------------------------------
vtkSMDomainIterator vtkSMDomainRange::begin() const
{
  return vtkSMDomainIterator(this->Vector.begin());
}

//------------------------------------------------------------------------------
vtkSMDomainIterator vtkSMDomainRange::end() const
{
  return vtkSMDomainIterator(this->Vector.end());
}
