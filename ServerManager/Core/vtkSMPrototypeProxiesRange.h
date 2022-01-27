/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPrototypeProxiesRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMPrototypeProxiesRange
 * @brief
 */
#ifndef vtkSMPrototypeProxiesRange_h
#define vtkSMPrototypeProxiesRange_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include "vtkSMPrototypeProxiesIterator.h" // for vtkSMPrototypeProxiesIterator

#include <map>     // for std::map
#include <string>  // for std::string
#include <utility> // for std::pair

class vtkSMProxy;

class VTKSERVERMANAGERCORE_EXPORT vtkSMPrototypeProxiesRange
{
public:
  using MapType = std::map<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>>;

  vtkSMPrototypeProxiesRange(MapType& map, const std::string& groupName = "");

  vtkSMPrototypeProxiesIterator begin() const;
  vtkSMPrototypeProxiesIterator end() const;

private:
  MapType& Map;
  std::string GroupName;
};
#endif
