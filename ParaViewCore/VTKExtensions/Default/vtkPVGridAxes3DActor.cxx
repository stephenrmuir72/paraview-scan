/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGridAxes3DActor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGridAxes3DActor.h"

#include "vtkBoundingBox.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"

#include <algorithm>

vtkStandardNewMacro(vtkPVGridAxes3DActor);
//----------------------------------------------------------------------------
vtkPVGridAxes3DActor::vtkPVGridAxes3DActor()
{
  this->DataScale[0] = this->DataScale[1] = this->DataScale[2] = 1.0;
  this->DataPosition[0] = this->DataPosition[1] = this->DataPosition[2] = 0.0;
  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;
  this->DataOrientation[0] = this->DataOrientation[1] = this->DataOrientation[2] = 0.0;
  this->DataBoundsInflateFactor = 0;

  this->TransformedBounds[0] = this->TransformedBounds[2] = this->TransformedBounds[4] = -1.0;
  this->TransformedBounds[1] = this->TransformedBounds[3] = this->TransformedBounds[5] = 1.0;

  this->UseModelTransform = false;
  this->ModelBounds[0] = this->ModelBounds[2] = this->ModelBounds[4] = -1.0;
  this->ModelBounds[1] = this->ModelBounds[3] = this->ModelBounds[5] = 1.0;
  this->ModelTransformMatrix->Identity();
}

//----------------------------------------------------------------------------
vtkPVGridAxes3DActor::~vtkPVGridAxes3DActor()
{
}

//----------------------------------------------------------------------------
double* vtkPVGridAxes3DActor::GetBounds()
{
  this->UpdateGridBounds();
  return this->Superclass::GetBounds();
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::SetModelTransformMatrix(double* matrix)
{
  if (!std::equal(matrix, matrix + 16, &this->ModelTransformMatrix->Element[0][0]))
  {
    std::copy(matrix, matrix + 16, &this->ModelTransformMatrix->Element[0][0]);
    this->ModelTransformMatrix->Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::Update(vtkViewport* viewport)
{
  this->UpdateGridBounds();
  this->Superclass::Update(viewport);
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::UpdateGridBounds()
{
  if (this->BoundsUpdateTime < this->GetMTime())
  {
    if (this->UseModelTransform)
    {
      this->UpdateGridBoundsUsingModelTransform();
    }
    else
    {
      this->UpdateGridBoundsUsingDataBounds();
    }
    this->BoundsUpdateTime.Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::UpdateGridBoundsUsingModelTransform()
{
  this->SetGridBounds(this->ModelBounds);
  this->SetOrigin(this->DataOrigin);
  this->SetOrientation(this->DataOrientation);
  this->SetPosition(this->DataPosition);
  this->SetScale(this->DataScale);
  this->SetUserMatrix(this->ModelTransformMatrix.Get());
}

void vtkPVGridAxes3DActor::ComputeMatrix()
{
  if (this->UseModelTransform)
  {
    if (this->IsIdentity)
    {
      return;
    }
    // check whether or not need to rebuild the matrix
    if (this->GetMTime() > this->MatrixMTime)
    {
      this->GetOrientation();
      this->Transform->Push();
      this->Transform->Identity();
      this->Transform->PostMultiply();

      // The DataSet is scaled after the ModelTransformMatrix has already been applied.
      // We need to change the order and scale the ModelTransformMatrix.
      if (this->UserTransform)
      {
        this->Transform->Concatenate(this->UserTransform->GetMatrix());
      }

      // shift back to actor's origin
      this->Transform->Translate(-this->Origin[0], -this->Origin[1], -this->Origin[2]);

      // scale
      this->Transform->Scale(this->Scale[0], this->Scale[1], this->Scale[2]);

      // rotate
      this->Transform->RotateY(this->Orientation[1]);
      this->Transform->RotateX(this->Orientation[0]);
      this->Transform->RotateZ(this->Orientation[2]);

      // move back from origin and translate
      this->Transform->Translate(this->Origin[0] + this->Position[0],
        this->Origin[1] + this->Position[1],
        this->Origin[2] + this->Position[2]);

      this->Transform->PreMultiply();
      this->Transform->GetMatrix(this->Matrix);
      this->Matrix->Modified();
      this->MatrixMTime.Modified();
      this->Transform->Pop();
    }
  }
  else
  {
    this->Superclass::ComputeMatrix();
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::UpdateGridBoundsUsingDataBounds()
{
  vtkBoundingBox bbox(this->TransformedBounds);
  if (bbox.IsValid())
  {
    if (this->DataPosition[0] != 0 || this->DataPosition[1] != 0 || this->DataPosition[2] != 0)
    {
      double bds[6];
      bbox.GetBounds(bds);
      bds[0] -= this->DataPosition[0];
      bds[1] -= this->DataPosition[0];
      bds[2] -= this->DataPosition[1];
      bds[3] -= this->DataPosition[1];
      bds[4] -= this->DataPosition[2];
      bds[5] -= this->DataPosition[2];
      bbox.SetBounds(bds);
      this->SetPosition(this->DataPosition);
    }
    else
    {
      this->SetPosition(0, 0, 0);
    }
    if (this->DataScale[0] != 1 || this->DataScale[1] != 1 || this->DataScale[2] != 1)
    {
      bbox.Scale(1.0 / this->DataScale[0], 1.0 / this->DataScale[1], 1.0 / this->DataScale[2]);
      this->SetScale(this->DataScale);
    }
    else
    {
      this->SetScale(1, 1, 1);
    }

    bbox.Inflate(bbox.GetDiagonalLength() * this->DataBoundsInflateFactor);

    double bds[6];
    bbox.GetBounds(bds);
    this->SetGridBounds(bds);
  }
  else
  {
    this->SetGridBounds(this->TransformedBounds);
    this->SetPosition(0, 0, 0);
    this->SetScale(1, 1, 1);
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::ShallowCopy(vtkProp* prop)
{
  this->Superclass::ShallowCopy(prop);
  if (vtkPVGridAxes3DActor* other = vtkPVGridAxes3DActor::SafeDownCast(prop))
  {
    this->SetDataScale(other->GetDataScale());
    this->SetDataPosition(other->GetDataPosition());
    this->SetDataOrigin(other->GetDataOrigin());
    this->SetDataOrientation(other->GetDataOrientation());
    this->SetTransformedBounds(other->GetTransformedBounds());
    this->SetUseModelTransform(other->GetUseModelTransform());
    this->SetModelBounds(other->GetModelBounds());
    this->SetModelTransformMatrix(reinterpret_cast<double*>(other->ModelTransformMatrix->Element));
  }
}

//----------------------------------------------------------------------------
void vtkPVGridAxes3DActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
