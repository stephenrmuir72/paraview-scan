/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJson.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkJson_h
#define vtkJson_h

#include "vtkJsonFwd.h"

// clang-format off
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

#define VTK_JSON_SAVE_MEMBER(json, x) json[#x] = x;

#define VTK_JSON_SAVE_MEMBER_ARRAY(json, x)                                                        \
  json[#x] =                                                                                       \
    std::vector<std::remove_extent<decltype(x)>::type>(x, x + std::extent<decltype(x)>::value);

#define VTK_JSON_LOAD_MEMBER(json, x) x = json.at(#x).get<decltype(x)>();

#define VTK_JSON_LOAD_MEMBER_ARRAY(json, x)                                                        \
  std::copy_n(json.at(#x).get<std::vector<std::remove_extent<decltype(x)>::type>>().begin(),       \
    std::extent<decltype(x)>::value, x);

#endif
