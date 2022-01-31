/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxyDefinitionsRange.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMProxyDefinitionsRange.h"
#include "vtkLogger.h"
vtkSMProxyDefinitionsRange::vtkSMProxyDefinitionsRange(MapType& map, const std::string& groupName)
  : Map(map)
  , GroupName(groupName)
{
}

//------------------------------------------------------------------------------
vtkSMProxyDefinitionsIterator vtkSMProxyDefinitionsRange::begin() const
{
  if (this->GroupName.empty())
  {
    return vtkSMProxyDefinitionsIterator(
      this->Map.begin(), this->Map.end(), this->Map.begin()->second.begin());
  }
  else
  {
    auto iterator = this->Map.find(this->GroupName);
    auto end = iterator;
    ++end;
    return vtkSMProxyDefinitionsIterator(iterator, end, iterator->second.begin());
  }
}

//------------------------------------------------------------------------------
vtkSMProxyDefinitionsIterator vtkSMProxyDefinitionsRange::end() const
{
  if (this->GroupName.empty())
  {
    return vtkSMProxyDefinitionsIterator(
      this->Map.end(), this->Map.end(), (--this->Map.end())->second.end());
  }
  else
  {
    auto iterator = this->Map.find(this->GroupName);
    auto end = iterator;
    ++end;
    return vtkSMProxyDefinitionsIterator(end, end, iterator->second.end());
  }
}
