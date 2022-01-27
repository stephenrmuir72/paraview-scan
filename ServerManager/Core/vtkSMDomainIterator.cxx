/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMDomainIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDomainIterator.h"

vtkSMDomainIterator::vtkSMDomainIterator(InternalIteratorType iterator)
{
  this->InternalIterator = iterator;
}

//------------------------------------------------------------------------------
bool vtkSMDomainIterator::operator==(const vtkSMDomainIterator& other) const
{
  return this->InternalIterator == other.InternalIterator;
}

//------------------------------------------------------------------------------
bool vtkSMDomainIterator::operator!=(const vtkSMDomainIterator& other) const
{
  return this->InternalIterator != other.InternalIterator;
}

//------------------------------------------------------------------------------
vtkSMDomainIterator vtkSMDomainIterator::operator->()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMDomainIterator vtkSMDomainIterator::operator*()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMDomainIterator& vtkSMDomainIterator::operator++()
{
  ++(this->InternalIterator);
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMDomainIterator vtkSMDomainIterator::operator++(int)
{
  auto result = vtkSMDomainIterator(this->InternalIterator);
  ++(*this);
  return result;
}

//------------------------------------------------------------------------------
std::string vtkSMDomainIterator::GetName() const
{
  return (*this->InternalIterator)->GetName();
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkSMDomain> vtkSMDomainIterator::GetDomain() const
{
  return (*this->InternalIterator);
}
