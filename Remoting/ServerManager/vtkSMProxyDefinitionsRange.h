/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxyDefinitionsRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMProxyDefinitionsRange
 * @brief
 */
#ifndef vtkSMProxyDefinitionsRange_h
#define vtkSMProxyDefinitionsRange_h

#include "vtkRemotingServerManagerModule.h" // for exports
#include <vtkSmartPointer.h>                // for vtkSmartPointer

#include "vtkSMProxyDefinitionsIterator.h" // for vtkSMProxyDefinitionsIterator

#include <map>     // for std::map
#include <memory>  // forst std::shared_ptr
#include <string>  // for std::string
#include <utility> // for std::pair

#include <vtk_pugixml.h>

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMProxyDefinitionsRange
{
public:
  using MapType = std::map<std::string, std::map<std::string, std::shared_ptr<pugi::xml_document>>>;

  vtkSMProxyDefinitionsRange(MapType& map, const std::string& groupName = "");

  vtkSMProxyDefinitionsIterator begin() const;
  vtkSMProxyDefinitionsIterator end() const;

private:
  MapType& Map;
  std::string GroupName;
};
#endif
