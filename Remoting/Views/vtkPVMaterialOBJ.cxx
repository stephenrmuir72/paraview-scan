/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterialOBJ.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMaterialOBJ.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMaterialOBJ);

//-----------------------------------------------------------------------------
void vtkPVMaterialOBJ::SetDiffuse(double r, double g, double b)
{
  // this->SetDoubleProperty("Kd", { r, g, b });
}

//-----------------------------------------------------------------------------
void vtkPVMaterialOBJ::SetSpecular(double r, double g, double b)
{
  // this->SetDoubleProperty("Ks", { r, g, b });
}

//-----------------------------------------------------------------------------
void vtkPVMaterialOBJ::SetShininess(double shininess)
{
  // this->SetDoubleProperty("Ns", { shininess });
}

//-----------------------------------------------------------------------------
void vtkPVMaterialOBJ::SetOpacity(double opacity)
{
  // this->SetDoubleProperty("d", { opacity });
}

//-----------------------------------------------------------------------------
void vtkPVMaterialOBJ::SetTransparencyFilter(double r, double g, double b)
{
  // this->SetDoubleProperty("Tf", { r, g, b });
}
