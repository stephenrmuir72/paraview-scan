/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMDomainRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMDomainRange
 * @brief
 */
#ifndef vtkSMDomainRange_h
#define vtkSMDomainRange_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include "vtkSMDomainIterator.h" // for vtkSMDomainIterator

#include <vector> // for std::vector

class vtkSMDomain;

class VTKSERVERMANAGERCORE_EXPORT vtkSMDomainRange
{
public:
  using VectorType = std::vector<vtkSmartPointer<vtkSMDomain>>;

  vtkSMDomainRange(vtkSmartPointer<vtkSMProperty> property, VectorType& vector);

  vtkSMDomainIterator begin() const;

  vtkSMDomainIterator end() const;

private:
  vtkSmartPointer<vtkSMProperty> Property;
  VectorType& Vector;
};
#endif
