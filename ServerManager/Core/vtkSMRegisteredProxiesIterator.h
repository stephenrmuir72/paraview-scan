/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMRegisteredProxiesIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMRegisteredProxiesIterator_h
#define vtkSMRegisteredProxiesIterator_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include <map>     // for std::mulitmap::iterator
#include <string>  // for std::string
#include <utility> // for std::pair

class vtkSMProxy;

class VTKSERVERMANAGERCORE_EXPORT vtkSMRegisteredProxiesIterator
{
public:
  using InternalIteratorType =
    std::multimap<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>>::iterator;

  vtkSMRegisteredProxiesIterator(InternalIteratorType iterator);

  bool operator==(const vtkSMRegisteredProxiesIterator& other) const;
  bool operator!=(const vtkSMRegisteredProxiesIterator& other) const;

  vtkSMRegisteredProxiesIterator operator->();
  vtkSMRegisteredProxiesIterator operator*();

  vtkSMRegisteredProxiesIterator& operator++();
  vtkSMRegisteredProxiesIterator operator++(int);

  std::string GetGroup() const;
  std::string GetName() const;

  vtkSmartPointer<vtkSMProxy> GetProxy() const;

private:
  InternalIteratorType InternalIterator;
};
#endif
