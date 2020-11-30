/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSharedUtils.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSharedUtils.h"

//----------------------------------------------------------------------------
float vtkSharedUtils::DegreesFromRadians(float x)
{
  return x * 57.2957795131f;
}

//----------------------------------------------------------------------------
double vtkSharedUtils::DegreesFromRadians(double x)
{
  return x * 57.29577951308232;
}
