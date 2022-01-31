/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMOrderedPropertyIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMOrderedPropertyIterator_h
#define vtkSMOrderedPropertyIterator_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include <map>    // for std::map::iterator
#include <string> // for std::string
#include <vector> // for std::vector::iterator

class vtkSMProperty;

class VTKSERVERMANAGERCORE_EXPORT vtkSMOrderedPropertyIterator
{
public:
  using MapType = std::map<std::string, vtkSmartPointer<vtkSMProperty>>;
  using InternalIteratorType = std::vector<std::string>::iterator;

  vtkSMOrderedPropertyIterator(const MapType& map, InternalIteratorType iterator);

  bool operator==(const vtkSMOrderedPropertyIterator& other) const;
  bool operator!=(const vtkSMOrderedPropertyIterator& other) const;

  vtkSMOrderedPropertyIterator operator->();
  vtkSMOrderedPropertyIterator operator*();

  vtkSMOrderedPropertyIterator& operator++();
  vtkSMOrderedPropertyIterator operator++(int);

  std::string GetName() const;

  vtkSmartPointer<vtkSMProperty> GetProperty() const;

private:
  const MapType& Map;
  InternalIteratorType InternalIterator;
};
#endif
