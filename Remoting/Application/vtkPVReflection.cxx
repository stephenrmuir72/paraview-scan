/*=========================================================================

  Program:   ParaView
  Module:    vtkPVReflection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVReflection.h"

#include "paraview_reflection.h"
#include "vtkObjectFactory.h"

vtkObjectFactoryNewMacro(vtkPVReflection);
//----------------------------------------------------------------------------
vtkPVReflection::vtkPVReflection() = default;

//----------------------------------------------------------------------------
vtkPVReflection::~vtkPVReflection() = default;

//----------------------------------------------------------------------------
void vtkPVReflection::Initialize()
{
  paraview_reflection_initialize();
}

//----------------------------------------------------------------------------
void vtkPVReflection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
