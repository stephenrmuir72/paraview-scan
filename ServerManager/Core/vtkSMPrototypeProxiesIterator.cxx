/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPrototypeProxiesIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPrototypeProxiesIterator.h"

vtkSMPrototypeProxiesIterator::vtkSMPrototypeProxiesIterator(InternalIteratorType iterator)
{
  this->InternalIterator = iterator;
}

//------------------------------------------------------------------------------
bool vtkSMPrototypeProxiesIterator::operator==(const vtkSMPrototypeProxiesIterator& other) const
{
  return this->InternalIterator == other.InternalIterator;
}

//------------------------------------------------------------------------------
bool vtkSMPrototypeProxiesIterator::operator!=(const vtkSMPrototypeProxiesIterator& other) const
{
  return this->InternalIterator != other.InternalIterator;
}

//------------------------------------------------------------------------------
vtkSMPrototypeProxiesIterator vtkSMPrototypeProxiesIterator::operator->()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMPrototypeProxiesIterator vtkSMPrototypeProxiesIterator::operator*()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMPrototypeProxiesIterator& vtkSMPrototypeProxiesIterator::operator++()
{
  ++(this->InternalIterator);
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMPrototypeProxiesIterator vtkSMPrototypeProxiesIterator::operator++(int)
{
  auto result = vtkSMPrototypeProxiesIterator(this->InternalIterator);
  ++(*this);
  return result;
}

//------------------------------------------------------------------------------
std::string vtkSMPrototypeProxiesIterator::GetGroup() const
{
  return this->InternalIterator->first.first;
}

//------------------------------------------------------------------------------
std::string vtkSMPrototypeProxiesIterator::GetName() const
{
  return this->InternalIterator->first.second;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkSMProxy> vtkSMPrototypeProxiesIterator::GetProxy() const
{
  return this->InternalIterator->second;
}
