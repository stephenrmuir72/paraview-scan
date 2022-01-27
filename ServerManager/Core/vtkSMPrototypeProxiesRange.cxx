/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPrototypeProxiesRange.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPrototypeProxiesRange.h"

vtkSMPrototypeProxiesRange::vtkSMPrototypeProxiesRange(MapType& map, const std::string& groupName)
  : Map(map)
  , GroupName(groupName)
{
}

//------------------------------------------------------------------------------
vtkSMPrototypeProxiesIterator vtkSMPrototypeProxiesRange::begin() const
{
  if (GroupName.empty())
  {
    return vtkSMPrototypeProxiesIterator(this->Map.begin());
  }
  else
  {
    // if a groupName is given we want to iterate only some of the elements
    // of the multimap.  Since the keys are std::pair<GroupName,ProxyName>
    // and the order is the default pairwise lexicographic order, elements of
    // the same group will be placed contigiously. This means that all
    // elements of the same group appear as a  sub-range of the map.
    // To extract this subrange we use std::lower_bound/std::upper_bound with
    // the min/max character value as second component of the key
    const std::string min_character(1, (char)(0));
    auto iterator = this->Map.lower_bound({ this->GroupName, min_character });

    return vtkSMPrototypeProxiesIterator(iterator);
  }
}

//------------------------------------------------------------------------------
vtkSMPrototypeProxiesIterator vtkSMPrototypeProxiesRange::end() const
{
  if (this->GroupName.empty())
  {
    return vtkSMPrototypeProxiesIterator(this->Map.end());
  }
  else
  {
    const std::string max_character(1, (char)(127));
    auto iterator = this->Map.upper_bound({ this->GroupName, max_character });

    return vtkSMPrototypeProxiesIterator(iterator);
  }
}
