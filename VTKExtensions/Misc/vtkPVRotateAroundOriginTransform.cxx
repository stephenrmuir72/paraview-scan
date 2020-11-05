/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRotateAroundOriginTransform

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRotateAroundOriginTransform.h"

#include "vtkObjectFactory.h"

#include <assert.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRotateAroundOriginTransform);
//----------------------------------------------------------------------------
vtkPVRotateAroundOriginTransform::vtkPVRotateAroundOriginTransform()
{
  this->AbsoluteTransform = vtkTransform::New();

  // Set origin of rotation to 0
  this->OriginOfRotation[0] = this->OriginOfRotation[1] = this->OriginOfRotation[2] = 0;

  // Set rotation to 0
  this->Rotation[0] = this->Rotation[1] = this->Rotation[2] = 0;
}

//----------------------------------------------------------------------------
vtkPVRotateAroundOriginTransform::~vtkPVRotateAroundOriginTransform()
{
  this->AbsoluteTransform->Delete();
}

//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::UpdateMatrix()
{
  this->AbsoluteTransform->Identity();

  this->AbsoluteTransform->Translate(this->OriginOfRotation);
  this->AbsoluteTransform->RotateZ(this->Rotation[2]);
  this->AbsoluteTransform->RotateX(this->Rotation[0]);
  this->AbsoluteTransform->RotateY(this->Rotation[1]);
  this->AbsoluteTransform->Translate(- this->OriginOfRotation[0], - this->OriginOfRotation[1], - this->OriginOfRotation[2]);

  // Delegate the setting to the vtkMatrix
  this->SetMatrix(this->AbsoluteTransform->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::SetOriginOfRotation(double xyz[3])
{
  this->SetOriginOfRotation(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::SetRotation(double xyz[3])
{
  this->SetRotation(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::SetOriginOfRotation(double x, double y, double z)
{
  this->OriginOfRotation[0] = x;
  this->OriginOfRotation[1] = y;
  this->OriginOfRotation[2] = z;

  UpdateMatrix(); // This will call modified... No need to do it twice
}
//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::SetRotation(double x, double y, double z)
{
  this->Rotation[0] = x;
  this->Rotation[1] = y;
  this->Rotation[2] = z;

  UpdateMatrix(); // This will call modified... No need to do it twice
}
