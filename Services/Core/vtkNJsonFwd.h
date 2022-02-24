/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNJsonFwd.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkNJsonFwd
 * @brief
 *
 */

#ifndef vtkNJsonFwd_h
#define vtkNJsonFwd_h

#include "vtkObject.h"

#include "vtk_nlohmannjson.h"
// clang-format off
#include VTK_NLOHMANN_JSON(json_fwd.hpp) // for json
// clang-format on

/**
 * Let's define a type for JSON to make it a little easier
 * to replace the implementation in future, if needed.
 */
using vtkNJson = nlohmann::json;

#endif
