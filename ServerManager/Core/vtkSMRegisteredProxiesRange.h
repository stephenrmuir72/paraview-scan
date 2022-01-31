/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMRegisteredProxiesRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMRegisteredProxiesRange
 * @brief
 */
#ifndef vtkSMRegisteredProxiesRange_h
#define vtkSMRegisteredProxiesRange_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include "vtkSMRegisteredProxiesIterator.h" // for vtkSMRegisteredProxiesIterator

#include <map>     // for std::multimap
#include <string>  // for std::string
#include <utility> // for std::pair

class vtkSMProxy;

class VTKSERVERMANAGERCORE_EXPORT vtkSMRegisteredProxiesRange
{
public:
  using MapType = std::multimap<std::pair<std::string, std::string>, vtkSmartPointer<vtkSMProxy>>;

  vtkSMRegisteredProxiesRange(MapType& map, const std::string& groupName = "");

  vtkSMRegisteredProxiesIterator begin() const;
  vtkSMRegisteredProxiesIterator end() const;

private:
  MapType& Map;
  std::string GroupName;
};
#endif
