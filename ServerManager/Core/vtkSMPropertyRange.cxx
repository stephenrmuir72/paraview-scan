/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPropertyRange.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPropertyRange.h"
#include "vtkSMProxy.h"

vtkSMPropertyRange::vtkSMPropertyRange(vtkSmartPointer<vtkSMProxy> proxy, MapType& map)
  : Map(map)
{
  this->Proxy = proxy;
}

//------------------------------------------------------------------------------
vtkSMPropertyIterator vtkSMPropertyRange::begin() const
{
  return vtkSMPropertyIterator(this->Map.begin());
}

//------------------------------------------------------------------------------
vtkSMPropertyIterator vtkSMPropertyRange::end() const
{
  return vtkSMPropertyIterator(this->Map.end());
}
