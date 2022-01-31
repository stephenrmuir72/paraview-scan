/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMRegisteredProxiesIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMRegisteredProxiesIterator.h"
vtkSMRegisteredProxiesIterator::vtkSMRegisteredProxiesIterator(InternalIteratorType iterator)
{
  this->InternalIterator = iterator;
}

//------------------------------------------------------------------------------
bool vtkSMRegisteredProxiesIterator::operator==(const vtkSMRegisteredProxiesIterator& other) const
{
  return this->InternalIterator == other.InternalIterator;
}

//------------------------------------------------------------------------------
bool vtkSMRegisteredProxiesIterator::operator!=(const vtkSMRegisteredProxiesIterator& other) const
{
  return this->InternalIterator != other.InternalIterator;
}

//------------------------------------------------------------------------------
vtkSMRegisteredProxiesIterator vtkSMRegisteredProxiesIterator::operator->()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMRegisteredProxiesIterator vtkSMRegisteredProxiesIterator::operator*()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMRegisteredProxiesIterator& vtkSMRegisteredProxiesIterator::operator++()
{
  ++(this->InternalIterator);
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMRegisteredProxiesIterator vtkSMRegisteredProxiesIterator::operator++(int)
{
  auto result = vtkSMRegisteredProxiesIterator(this->InternalIterator);
  ++(*this);
  return result;
}

//------------------------------------------------------------------------------
std::string vtkSMRegisteredProxiesIterator::GetGroup() const
{
  return this->InternalIterator->first.first;
}

//------------------------------------------------------------------------------
std::string vtkSMRegisteredProxiesIterator::GetName() const
{
  return this->InternalIterator->first.second;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkSMProxy> vtkSMRegisteredProxiesIterator::GetProxy() const
{
  return this->InternalIterator->second;
}
