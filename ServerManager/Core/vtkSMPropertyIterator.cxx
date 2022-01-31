/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPropertyIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPropertyIterator.h"

vtkSMPropertyIterator::vtkSMPropertyIterator(vtkSMPropertyIterator::InternalIteratorType iterator)
{
  this->InternalIterator = iterator;
}

//------------------------------------------------------------------------------
bool vtkSMPropertyIterator::operator==(const vtkSMPropertyIterator& other) const
{
  return this->InternalIterator == other.InternalIterator;
}

//------------------------------------------------------------------------------
bool vtkSMPropertyIterator::operator!=(const vtkSMPropertyIterator& other) const
{
  return this->InternalIterator != other.InternalIterator;
}

//------------------------------------------------------------------------------
vtkSMPropertyIterator vtkSMPropertyIterator::operator->()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMPropertyIterator vtkSMPropertyIterator::operator*()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMPropertyIterator& vtkSMPropertyIterator::operator++()
{
  ++(this->InternalIterator);
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMPropertyIterator vtkSMPropertyIterator::operator++(int)
{
  auto result = vtkSMPropertyIterator(this->InternalIterator);
  ++(*this);
  return result;
}

//------------------------------------------------------------------------------
std::string vtkSMPropertyIterator::GetName() const
{
  return this->InternalIterator->first;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkSMProperty> vtkSMPropertyIterator::GetProperty() const
{
  return this->InternalIterator->second;
}
