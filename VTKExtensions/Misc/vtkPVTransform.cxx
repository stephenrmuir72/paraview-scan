/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTransform

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTransform.h"

#include "vtkObjectFactory.h"

#include <assert.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTransform);
//----------------------------------------------------------------------------
vtkPVTransform::vtkPVTransform()
{
  this->AbsoluteTransform = vtkTransform::New();

  // Set position to 0
  this->AbsolutePosition[0] = this->AbsolutePosition[1] = this->AbsolutePosition[2] = 0;

  // Set rotation to 0
  this->AbsoluteRotation[0] = this->AbsoluteRotation[1] = this->AbsoluteRotation[2] = 0;

  // Set scale to 1
  this->AbsoluteScale[0] = this->AbsoluteScale[1] = this->AbsoluteScale[2] = 1;

  // Set origin of rotation to 0
  this->RotationOrigin[0] = this->RotationOrigin[1] = this->RotationOrigin[2] = 0;
}

//----------------------------------------------------------------------------
vtkPVTransform::~vtkPVTransform()
{
  this->AbsoluteTransform->Delete();
}

//----------------------------------------------------------------------------
void vtkPVTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AbsolutePosition: "
     << "( " << this->AbsolutePosition[0] << ", " << this->AbsolutePosition[1] << ", "
     << this->AbsolutePosition[2] << ")\n";

  os << indent << "AbsoluteRotation: "
     << "( " << this->AbsoluteRotation[0] << ", " << this->AbsoluteRotation[1] << ", "
     << this->AbsoluteRotation[2] << ")\n";

  os << indent << "AbsoluteScale: "
     << "( " << this->AbsoluteScale[0] << ", " << this->AbsoluteScale[1] << ", "
     << this->AbsoluteScale[2] << ")\n";

  os << indent << "RotationOrigin: "
     << "( " << this->RotationOrigin[0] << ", " << this->RotationOrigin[1] << ", "
     << this->RotationOrigin[2] << ")\n";
}

//----------------------------------------------------------------------------
void vtkPVTransform::UpdateMatrix()
{
  this->AbsoluteTransform->Identity();
  this->AbsoluteTransform->Translate(this->AbsolutePosition);
  this->AbsoluteTransform->RotateZ(this->AbsoluteRotation[2]);
  this->AbsoluteTransform->RotateX(this->AbsoluteRotation[0]);
  this->AbsoluteTransform->RotateY(this->AbsoluteRotation[1]);
  this->AbsoluteTransform->Scale(this->AbsoluteScale);

  // Delegate the setting to the vtkMatrix
  this->SetMatrix(this->AbsoluteTransform->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsolutePosition(double xyz[3])
{
  this->SetAbsolutePosition(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteRotation(double xyz[3])
{
  this->SetAbsoluteRotation(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteScale(double xyz[3])
{
  this->SetAbsoluteScale(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteRotationAroundOrigin(double xyz[3])
{
  this->SetAbsoluteRotationAroundOrigin(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsolutePosition(double x, double y, double z)
{
  this->AbsolutePosition[0] = x;
  this->AbsolutePosition[1] = y;
  this->AbsolutePosition[2] = z;

  std::cout << "SetAbsolutePosition :" << x << std::endl;

  UpdateMatrix(); // This will call modified... No need to do it twice
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteRotation(double x, double y, double z)
{
  this->AbsoluteRotation[0] = x;
  this->AbsoluteRotation[1] = y;
  this->AbsoluteRotation[2] = z;

  std::cout << "SetAbsoluteRotation :" << x << std::endl;

  UpdateMatrix(); // This will call modified... No need to do it twice
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteScale(double x, double y, double z)
{
  this->AbsoluteScale[0] = x;
  this->AbsoluteScale[1] = y;
  this->AbsoluteScale[2] = z;

  std::cout << "SetAbsoluteScale :" << x << std::endl;

  UpdateMatrix(); // This will call modified... No need to do it twice
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetRotationOrigin(double xyz[3])
{
  this->SetRotationOrigin(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetRotationOrigin(double x, double y, double z)
{
  this->RotationOrigin[0] = x;
  this->RotationOrigin[1] = y;
  this->RotationOrigin[2] = z;

  //UpdateMatrix();
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteRotationAroundOrigin(double x, double y, double z)
{
  this->AbsoluteRotationAroundOrigin[0] = x;
  this->AbsoluteRotationAroundOrigin[1] = y;
  this->AbsoluteRotationAroundOrigin[2] = z;

  std::cout << "SetAbsoluteRotationAroundOrigin :" << x << std::endl;


  this->AbsoluteTransform->Identity();
  this->AbsoluteTransform->Translate(this->AbsolutePosition);
  this->AbsoluteTransform->RotateZ(this->AbsoluteRotation[2]);
  this->AbsoluteTransform->RotateX(this->AbsoluteRotation[0]);
  this->AbsoluteTransform->RotateY(this->AbsoluteRotation[1]);

  this->AbsoluteTransform->Translate(this->RotationOrigin);
  this->AbsoluteTransform->RotateZ(this->AbsoluteRotationAroundOrigin[2]);
  this->AbsoluteTransform->RotateX(this->AbsoluteRotationAroundOrigin[0]);
  this->AbsoluteTransform->RotateY(this->AbsoluteRotationAroundOrigin[1]);
  this->AbsoluteTransform->Translate(- this->RotationOrigin[0], - this->RotationOrigin[1], - this->RotationOrigin[2]);

  //this->AbsoluteTransform->Scale(this->AbsoluteScale);

  double pos[3] = {0.0,0.0,0.0};
  double resPos[3] = {0.0,0.0,0.0};

  this->AbsoluteTransform->TransformPoint(pos, resPos);

  this->SetAbsolutePosition(resPos);

  //UpdateMatrix(); // This will call modified... No need to do it twice
}

void vtkPVTransform::GetPositionWithoutOrigin(double pos[3])
{
  std::cout << "GetPositionWithoutOrigin" << std::endl;

  // Save position origin
  // double rOr[3] = { this->RotationOrigin[0], this->RotationOrigin[1], this->RotationOrigin[2] };

  //this->SetRotationOrigin(0.0, 0.0, 0.0);
  //this->UpdateMatrix();
  //vtkMatrix4x4 *mat = this->AbsoluteTransform->GetMatrix();
  // pos[0] = mat->Element[0][3];
  // pos[1] = mat->Element[1][3];
  // pos[2] = mat->Element[2][3];

  // double cPos[3] = {0.0,0.0,0.0};
  // this->AbsoluteTransform->TransformPoint(cPos, pos);


  // this->SetRotationOrigin(0.0,0.0,0.0);
  // std::cout << this->AbsoluteRotation[0] << std::endl;



  // this->GetPosition(pos);



  // if (rOr[0] == 0.0 && rOr[1] == 0.0 && rOr[2] == 0.0)
  // {
  //   this->GetPosition(pos);
  // }
  // else
  // {

  //   // Modify pos with the rotation around an other origin

  //   this->UpdateMatrix();

  //   this->GetPosition(pos);

  //   this->AbsolutePosition[0] = pos[0];
  //   this->AbsolutePosition[1] = pos[1];
  //   this->AbsolutePosition[2] = pos[2];

  //   this->UpdateMatrix();

  // }





  pos[0] = this->AbsolutePosition[0];
  pos[1] = this->AbsolutePosition[1];
  pos[2] = this->AbsolutePosition[2];

}

void vtkPVTransform::GetOrientation2(double orient[3])
{
  std::cout << "GetOrientation2" << std::endl;

  // this->SetMatrix(this->AbsoluteTransform->GetMatrix());

  //this->UpdateMatrix(false);

  this->GetOrientation(orient);

  std::cout << "AHAHHAHAHAHAHHA:" << this->RotationOrigin[0] << std::endl;

  double rOr[3] = { this->RotationOrigin[0], this->RotationOrigin[1], this->RotationOrigin[2] };
  if (rOr[0] == 0.0 || rOr[1] == 0.0 || rOr[2] == 0.0)
  {


    const double pos[3] = { this->AbsolutePosition[0],
                      this->AbsolutePosition[1],
                      this->AbsolutePosition[2] };

    this->AbsoluteTransform->Identity();
    this->AbsoluteTransform->Translate(this->RotationOrigin);
    this->AbsoluteTransform->RotateZ(this->AbsoluteRotation[2]);
    this->AbsoluteTransform->RotateX(this->AbsoluteRotation[0]);
    this->AbsoluteTransform->RotateY(this->AbsoluteRotation[1]);
    this->AbsoluteTransform->Translate(
      -this->RotationOrigin[0], -this->RotationOrigin[1], -this->RotationOrigin[2]);


    double resPos[3] = {0.0,0.0,0.0};

    this->AbsoluteTransform->TransformPoint(pos, resPos);

    std::cout << "aha :: " << resPos[0] << std::endl; 
      


    //this->SetAbsolutePosition(resPos);

    // this->SetAbsolutePosition(pos);

    this->AbsolutePosition[0] = pos[0];
    this->AbsolutePosition[1] = pos[1];
    this->AbsolutePosition[2] = pos[2];
    this->UpdateMatrix();

  }

}
