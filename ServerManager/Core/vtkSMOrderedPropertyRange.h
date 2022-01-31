/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMOrderedPropertyRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMOrderedPropertyRange
 * @brief
 */
#ifndef vtkSMOrderedPropertyRange_h
#define vtkSMOrderedPropertyRange_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include "vtkSMOrderedPropertyIterator.h" // for vtkSMOrderedPropertyIterator

#include <map>    // for std::map
#include <string> // for std::string
#include <vector> // for std::vector

class vtkSMProxy;

class VTKSERVERMANAGERCORE_EXPORT vtkSMOrderedPropertyRange
{
public:
  using MapType = std::map<std::string, vtkSmartPointer<vtkSMProperty>>;
  using VectorType = std::vector<std::string>;

  vtkSMOrderedPropertyRange(vtkSmartPointer<vtkSMProxy> proxy, MapType& map, VectorType& vector);

  vtkSMOrderedPropertyIterator begin() const;
  vtkSMOrderedPropertyIterator end() const;

private:
  vtkSmartPointer<vtkSMProxy> Proxy;
  MapType& Map;
  VectorType& Vector;
};
#endif
