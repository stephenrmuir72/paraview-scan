/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxyDefinitionsIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMProxyDefinitionsIterator.h"
#include "vtkLogger.h"

vtkSMProxyDefinitionsIterator::vtkSMProxyDefinitionsIterator(
  Level0InternalIteratorType level0IteratorCurrent, Level0InternalIteratorType level0IteratorEnd,
  Level1InternalIteratorType level1Iterator)
{
  this->InternalLevel0IteratorCurrent = level0IteratorCurrent;
  this->InternalLevel0IteratorEnd = level0IteratorEnd;
  this->InternalLevel1Iterator = level1Iterator;
}

//------------------------------------------------------------------------------
bool vtkSMProxyDefinitionsIterator::operator==(const vtkSMProxyDefinitionsIterator& other) const
{
  return this->InternalLevel0IteratorCurrent == other.InternalLevel0IteratorCurrent &&
    this->InternalLevel1Iterator == other.InternalLevel1Iterator;
}

//------------------------------------------------------------------------------
bool vtkSMProxyDefinitionsIterator::operator!=(const vtkSMProxyDefinitionsIterator& other) const
{
  return this->InternalLevel0IteratorCurrent != other.InternalLevel0IteratorCurrent ||
    this->InternalLevel1Iterator != other.InternalLevel1Iterator;
}

//------------------------------------------------------------------------------
vtkSMProxyDefinitionsIterator vtkSMProxyDefinitionsIterator::operator->()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMProxyDefinitionsIterator vtkSMProxyDefinitionsIterator::operator*()
{
  return (*this);
}

//------------------------------------------------------------------------------
vtkSMProxyDefinitionsIterator& vtkSMProxyDefinitionsIterator::operator++()
{
  ++(this->InternalLevel1Iterator);
  if (this->InternalLevel1Iterator == this->InternalLevel0IteratorCurrent->second.end())
  {
    ++(this->InternalLevel0IteratorCurrent);
    if (this->InternalLevel0IteratorCurrent != this->InternalLevel0IteratorEnd)
    {
      this->InternalLevel1Iterator = this->InternalLevel0IteratorCurrent->second.begin();
    }
  }

  return (*this);
}

//------------------------------------------------------------------------------
vtkSMProxyDefinitionsIterator vtkSMProxyDefinitionsIterator::operator++(int)
{
  auto result = vtkSMProxyDefinitionsIterator(this->InternalLevel0IteratorCurrent,
    this->InternalLevel0IteratorEnd, this->InternalLevel1Iterator);
  ++(*this);
  return result;
}

//------------------------------------------------------------------------------
std::string vtkSMProxyDefinitionsIterator::GetGroup() const
{
  return this->InternalLevel0IteratorCurrent->first;
}

//------------------------------------------------------------------------------
std::string vtkSMProxyDefinitionsIterator::GetName() const
{
  return this->InternalLevel1Iterator->first;
}

//------------------------------------------------------------------------------
std::shared_ptr<pugi::xml_document> vtkSMProxyDefinitionsIterator::GetDefinition() const
{
  return this->InternalLevel1Iterator->second;
}
