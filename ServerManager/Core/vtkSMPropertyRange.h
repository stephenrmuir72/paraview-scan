/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPropertyRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMPropertyRange
 * @brief
 */
#ifndef vtkSMPropertyRange_h
#define vtkSMPropertyRange_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include "vtkSMPropertyIterator.h" // for vtkSMPropertyIterator

#include <map>    // for std::map
#include <string> // for std::string

class vtkSMProxy;

class VTKSERVERMANAGERCORE_EXPORT vtkSMPropertyRange
{
public:
  using MapType = std::map<std::string, vtkSmartPointer<vtkSMProperty>>;

  vtkSMPropertyRange(vtkSmartPointer<vtkSMProxy> proxy, MapType& map);

  vtkSMPropertyIterator begin() const;
  vtkSMPropertyIterator end() const;

private:
  vtkSmartPointer<vtkSMProxy> Proxy;
  MapType& Map;
};
#endif
