/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPropertyIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMPropertyIterator_h
#define vtkSMPropertyIterator_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include <map>    // for map::iterator
#include <string> // for std::string

class vtkSMProperty;

class VTKSERVERMANAGERCORE_EXPORT vtkSMPropertyIterator
{
public:
  using InternalIteratorType = std::map<std::string, vtkSmartPointer<vtkSMProperty>>::iterator;

  vtkSMPropertyIterator(InternalIteratorType iterator);

  bool operator==(const vtkSMPropertyIterator& other) const;

  bool operator!=(const vtkSMPropertyIterator& other) const;

  vtkSMPropertyIterator operator->();
  vtkSMPropertyIterator operator*();

  vtkSMPropertyIterator& operator++();
  vtkSMPropertyIterator operator++(int);

  std::string GetName() const;

  vtkSmartPointer<vtkSMProperty> GetProperty() const;

private:
  InternalIteratorType InternalIterator;
};
#endif
