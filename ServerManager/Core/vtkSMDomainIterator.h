/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMDomainIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMDomainIterator_h
#define vtkSMDomainIterator_h

#include "vtkSMDomain.h"
#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include <vector> // for std::vector::iterator

class VTKSERVERMANAGERCORE_EXPORT vtkSMDomainIterator
{
public:
  using InternalIteratorType = std::vector<vtkSmartPointer<vtkSMDomain>>::iterator;

  vtkSMDomainIterator(InternalIteratorType iterator);

  bool operator==(const vtkSMDomainIterator& other) const;
  bool operator!=(const vtkSMDomainIterator& other) const;

  vtkSMDomainIterator operator->();
  vtkSMDomainIterator operator*();

  vtkSMDomainIterator& operator++();
  vtkSMDomainIterator operator++(int);

  std::string GetName() const;
  vtkSmartPointer<vtkSMDomain> GetDomain() const;

private:
  InternalIteratorType InternalIterator;
};
#endif
