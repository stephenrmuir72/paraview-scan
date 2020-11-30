/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoreUtils.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCoreUtils.h"

//----------------------------------------------------------------------------
float vtkCoreUtils::RadiansFromDegrees(float x)
{
  return x * 0.017453292f;
}

//----------------------------------------------------------------------------
double vtkCoreUtils::RadiansFromDegrees(double x)
{
  return x * 0.017453292519943295;
}
