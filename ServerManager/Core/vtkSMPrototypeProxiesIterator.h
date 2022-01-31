/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPrototypeProxiesIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMPrototypeProxiesIterator_h
#define vtkSMPrototypeProxiesIterator_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include <map>     // for map::iterator
#include <string>  // for std::string
#include <utility> // for std::pair

class vtkSMProxy;

class VTKSERVERMANAGERCORE_EXPORT vtkSMPrototypeProxiesIterator
{
public:
  using InternalIteratorType =
    std::map<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>>::iterator;

  vtkSMPrototypeProxiesIterator(InternalIteratorType iterator);

  bool operator==(const vtkSMPrototypeProxiesIterator& other) const;
  bool operator!=(const vtkSMPrototypeProxiesIterator& other) const;

  vtkSMPrototypeProxiesIterator operator->();
  vtkSMPrototypeProxiesIterator operator*();

  vtkSMPrototypeProxiesIterator& operator++();
  vtkSMPrototypeProxiesIterator operator++(int);

  std::string GetGroup() const;
  std::string GetName() const;

  vtkSmartPointer<vtkSMProxy> GetProxy() const;

private:
  InternalIteratorType InternalIterator;
};
#endif
