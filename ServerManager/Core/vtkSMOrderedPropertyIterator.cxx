/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMOrderedPropertyIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMOrderedPropertyIterator.h"

#include <vtkSMProperty.h>

#include <map> // for std::map::iterator

vtkSMOrderedPropertyIterator::vtkSMOrderedPropertyIterator(
  const MapType& map, InternalIteratorType iterator)
  : Map(map)
  , InternalIterator(iterator)
{
}

bool vtkSMOrderedPropertyIterator::operator==(const vtkSMOrderedPropertyIterator& other) const
{
  return this->InternalIterator == other.InternalIterator;
}
bool vtkSMOrderedPropertyIterator::operator!=(const vtkSMOrderedPropertyIterator& other) const
{
  return this->InternalIterator != other.InternalIterator;
}

vtkSMOrderedPropertyIterator vtkSMOrderedPropertyIterator::operator->()
{
  return (*this);
}
vtkSMOrderedPropertyIterator vtkSMOrderedPropertyIterator::operator*()
{
  return (*this);
}

vtkSMOrderedPropertyIterator& vtkSMOrderedPropertyIterator::operator++()
{
  ++(this->InternalIterator);
  return (*this);
}

vtkSMOrderedPropertyIterator vtkSMOrderedPropertyIterator::operator++(int)
{
  auto result = vtkSMOrderedPropertyIterator(this->Map, this->InternalIterator);
  ++(*this);
  return result;
}

std::string vtkSMOrderedPropertyIterator::GetName() const
{
  return (*this->InternalIterator);
}

vtkSmartPointer<vtkSMProperty> vtkSMOrderedPropertyIterator::GetProperty() const
{
  const std::string& key = this->GetName();
  return this->Map.at(key);
}
