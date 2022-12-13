/*=========================================================================

   Program: ParaView
   Module:  vtkSMVRGrabTransformStyleProxy.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "vtkSMVRGrabTransformStyleProxy.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"
#include "pqView.h"

#include <algorithm>
#include <cmath>
#include <sstream>

// WRS: This version of "vtkSMVRGrabTransformStyleProxy.cxx" contains code that will
//   output information into a file (or pipe), that allows an helper tool to
//   visualize the interactions that are taking place.  The purpose of this
//   is to help debug the order of transformations performed by this style.

#define OUTPUT_DATA_FOR_DEBUGGING
#ifdef OUTPUT_DATA_FOR_DEBUGGING
// ----------------------------------------------------------------------------
// sendMatrixToPVtest(): Output a 4x4 matrix to produce a graphical representation
// Parameters:
//   freq -- whether or not to output frequently (all), or infrequently, of this marker (to help
//   with data flow)
void sendMatrixToPVtest(int marker, int freq, vtkMatrix4x4* mat)
{
  static int counter = 0;

  if (!freq)
  {
    counter++;
    if (counter == (53 - 1))
    {
      counter = 1;
    }
    else
    {
      return;
    }
  }
  printf("pos %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n", marker, mat->Element[0][0],
    mat->Element[1][0], mat->Element[2][0], mat->Element[3][0], mat->Element[0][1],
    mat->Element[1][1], mat->Element[2][1], mat->Element[3][1], mat->Element[0][2],
    mat->Element[1][2], mat->Element[2][2], mat->Element[3][2], mat->Element[0][3],
    mat->Element[1][3], mat->Element[2][3], mat->Element[3][3]);
  fflush(stdout);
}

// ----------------------------------------------------------------------------
// sendLocationToPVtest(): Output a 3 element array to produce a graphical representation
//   freq -- whether or not to output frequently (all), or infrequently, of this marker (to help
//   with data flow)
void sendLocationToPVtest(int marker, int freq, float x, float y, float z)
{
  static int counter = 0;

  if (!freq)
  {
    counter++;
    if (counter == (53 - 1))
    {
      counter = 1;
    }
    else
    {
      return;
    }
  }
  printf("loc %d %f %f %f\n", marker, x, y, z);
  fflush(stdout);
}
#endif /* OUTPUT_DATA_FOR_DEBUGGING */

class vtkSMVRGrabTransformStyleProxy::vtkInternals
{
public:
  double GetSpeed(vtkSMProxy* proxy)
  {
    vtkSMDoubleVectorProperty* dvp;
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty("Speed"));
    return dvp->GetElement(0);
  }
};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRGrabTransformStyleProxy);

// ----------------------------------------------------------------------------
// Constructor method
vtkSMVRGrabTransformStyleProxy::vtkSMVRGrabTransformStyleProxy()
  : Superclass()
{
  this->Internals = new vtkInternals();

  this->AddButtonRole("Navigate world");
  this->AddButtonRole("Reset world");
  this->EnableNavigate = false;
  this->IsInitialRecorded = false;
}

// ----------------------------------------------------------------------------
// Destructor method
vtkSMVRGrabTransformStyleProxy::~vtkSMVRGrabTransformStyleProxy()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkSMVRGrabTransformStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableNavigate: " << this->EnableNavigate << endl;
  os << indent << "IsInitialRecorded: " << this->IsInitialRecorded << endl;
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkSMVRGrabTransformStyleProxy::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);
  if (role == "Navigate world")
  {
    vtkCamera* camera = nullptr;
    this->EnableNavigate = event.data.button.state;
    if (!this->EnableNavigate)
    {
      this->IsInitialRecorded = false;
    }
  }
  else if (event.data.button.state && role == "Reset world")
  {
    if (this->ControlledProxy == nullptr)
    {
      vtkErrorMacro("No controlled proxy set!");
      return;
    }
    vtkNew<vtkMatrix4x4> transformMatrix;

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(&transformMatrix.GetPointer()->Element[0][0], 16);
    this->ControlledProxy->UpdateVTKObjects();

    this->IsInitialRecorded = false;
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRGrabTransformStyleProxy::HandleTracker(const vtkVREvent& event)
{
  static double lastspeed = -1.0;

  vtkCamera* camera = nullptr;
  std::string role = this->GetTrackerRole(event.name);

  if (role != "Tracker")
    return;

  if (this->ControlledProxy == nullptr)
  {
    vtkErrorMacro("No controlled proxy set!");
    return;
  }

  if (this->EnableNavigate)
  {
    if (!this->IsInitialRecorded)
    {
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
        .Get(&(this->SavedModelViewMatrix->Element[0][0]), 16);

      this->SavedInverseWandMatrix->DeepCopy(event.data.tracker.matrix);

      double speed = this->Internals->GetSpeed(this);
      if (speed != 1.0)
      {
        std::cout << "GrabTransform using speed " << speed << std::endl;
        this->SavedInverseWandMatrix->SetElement(
          0, 3, this->SavedInverseWandMatrix->GetElement(0, 3) * speed);
        this->SavedInverseWandMatrix->SetElement(
          1, 3, this->SavedInverseWandMatrix->GetElement(1, 3) * speed);
        this->SavedInverseWandMatrix->SetElement(
          2, 3, this->SavedInverseWandMatrix->GetElement(2, 3) * speed);
      }

      vtkMatrix4x4::Invert(this->SavedInverseWandMatrix, this->SavedInverseWandMatrix);

      this->IsInitialRecorded = true;
    }
    else
    {
      // Apply the transformation and get the new transformation matrix
      vtkNew<vtkMatrix4x4> transformMatrix;
      transformMatrix->DeepCopy(event.data.tracker.matrix);

      double speed = this->Internals->GetSpeed(this);
      if (speed != 1.0)
      {
        std::cout << "GrabTransform using speed " << speed << std::endl;
        transformMatrix->SetElement(0, 3, transformMatrix->GetElement(0, 3) * speed);
        transformMatrix->SetElement(1, 3, transformMatrix->GetElement(1, 3) * speed);
        transformMatrix->SetElement(2, 3, transformMatrix->GetElement(2, 3) * speed);
      }

      // swapping order of both multiplies
      vtkMatrix4x4::Multiply4x4(transformMatrix, this->SavedInverseWandMatrix, transformMatrix);
      vtkMatrix4x4::Multiply4x4(transformMatrix, this->SavedModelViewMatrix, transformMatrix);

      // Set the new matrix for the proxy.
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
        .Set(&(transformMatrix->Element[0][0]), 16);
      this->ControlledProxy->UpdateVTKObjects();
    }
  }
}
