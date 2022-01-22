/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessInitializer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProcessInitializer.h"
#include "paraview_reflection.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkProcessInitializer);
//----------------------------------------------------------------------------
vtkProcessInitializer::vtkProcessInitializer() = default;
//----------------------------------------------------------------------------
vtkProcessInitializer::~vtkProcessInitializer() = default;
//----------------------------------------------------------------------------
void vtkProcessInitializer::InitializeReflection()
{
  paraview_reflection_initialize();
}
