/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMOrderedPropertyRange.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMOrderedPropertyRange.h"

#include "vtkSMProxy.h"

vtkSMOrderedPropertyRange::vtkSMOrderedPropertyRange(
  vtkSmartPointer<vtkSMProxy> proxy, MapType& map, VectorType& vector)
  : Map(map)
  , Vector(vector)
{
  this->Proxy = proxy;
}

vtkSMOrderedPropertyIterator vtkSMOrderedPropertyRange::begin() const
{
  return vtkSMOrderedPropertyIterator(this->Map, this->Vector.begin());
}

vtkSMOrderedPropertyIterator vtkSMOrderedPropertyRange::end() const
{
  return vtkSMOrderedPropertyIterator(this->Map, this->Vector.end());
}
