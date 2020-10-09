/*=========================================================================

  Program:   ParaView
  Module:    vtkPVZSpaceView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVZSpaceView.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCullerCollection.h"
#include "vtkGeometryRepresentation.h"
#include "vtkMapper.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkRenderWindow.h"

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkProperty.h"

#include <zSpace.h>

#include <Windows.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVZSpaceView);

#define CHECK_ERROR(fn, error)                                                                     \
  if (error != ZC_ERROR_OK)                                                                        \
  {                                                                                                \
    char errorString[256];                                                                         \
    zcGetErrorString(error, errorString, sizeof(errorString));                                     \
    MessageBox(NULL, errorString, #fn, MB_OK | MB_SETFOREGROUND | MB_TOPMOST);                     \
  }

//----------------------------------------------------------------------------
vtkPVZSpaceView::vtkPVZSpaceView()
{
  this->InitializeZSpace();
  this->GetRenderer()->GetCullers()->RemoveAllItems();

  vtkMemberFunctionCommand<vtkPVZSpaceView>* observer =
    vtkMemberFunctionCommand<vtkPVZSpaceView>::New();
  observer->SetCallback(*this, &vtkPVZSpaceView::ResetCamera);
  this->AddObserver(vtkCommand::ResetCameraEvent, observer);
  observer->FastDelete();
}

void vtkPVZSpaceView::ResetCamera()
{
  this->UpdateZSpaceFrustumParameters(true);
}

vtkPVZSpaceView::~vtkPVZSpaceView()
{
  zcDestroyStereoBuffer(this->BufferHandle);
  zcDestroyViewport(this->ViewportHandle);
  zcShutDown(this->ZSpaceContext);
}

void vtkPVZSpaceView::ResetCameraClippingRange()
{
  this->Superclass::ResetCameraClippingRange();
  this->UpdateZSpaceFrustumParameters();
}

void vtkPVZSpaceView::InitializeZSpace()
{
  ZCError error;

  // Initialize the zSpace SDK. This MUST be called before
  // calling any other zSpace API.
  error = zcInitialize(&this->ZSpaceContext);
  CHECK_ERROR(zcInitialize, error);

  int numDisplays;
  error = zcGetNumDisplays(this->ZSpaceContext, &numDisplays);

  this->Displays.reserve(numDisplays);

  for (int i = 0; i < numDisplays; i++)
  {
    ZCHandle displayHandle;
    zcGetDisplayByIndex(this->ZSpaceContext, i, &displayHandle);

    ZCDisplayType displayType;
    zcGetDisplayType(displayHandle, &displayType);

    switch (displayType)
    {
      case ZC_DISPLAY_TYPE_UNKNOWN:
        this->Displays.push_back("Unknown");
        break;
      case ZC_DISPLAY_TYPE_GENERIC:
        this->Displays.push_back("Generic");
        break;
      case ZC_DISPLAY_TYPE_ZSPACE:
        this->Displays.push_back("ZSpace");
        break;
      default:
        this->Displays.push_back("???");
        break;
    }
  }

  // Create a stereo buffer to handle L/R detection.
  error =
    zcCreateStereoBuffer(this->ZSpaceContext, ZC_RENDERER_QUAD_BUFFER_GL, 0, &this->BufferHandle);
  CHECK_ERROR(zcCreateStereoBuffer, error);

  // Create a zSpace viewport object and grab its associated frustum.
  // Note: The zSpace viewport is abstract and not an actual window/viewport
  // that is created and registered through the Windows OS. It manages
  // a zSpace stereo frustum, which is responsible for various stereoscopic
  // 3D calculations such as calculating the view and projection matrices for
  // each eye.
  error = zcCreateViewport(this->ZSpaceContext, &this->ViewportHandle);
  CHECK_ERROR(zcCreateViewport, error);

  error = zcGetFrustum(this->ViewportHandle, &this->FrustumHandle);
  CHECK_ERROR(zcGetFrustum, error);

  // Enable auto stereo.
  zcSetFrustumAttributeB(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_AUTO_STEREO_ENABLED, true);
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_IPD, 0.056f);
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_HEAD_SCALE, 1.f);

  zcSetFrustumPortalMode(this->FrustumHandle, ZC_PORTAL_MODE_NONE);

  error = zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_PRIMARY, &this->StylusTargets);
  CHECK_ERROR(zcGetNumTargetsByType, error);

  error = zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_HEAD, &this->HeadTargets);
  CHECK_ERROR(zcGetNumTargetsByType, error);

  error =
    zcGetNumTargetsByType(this->ZSpaceContext, ZC_TARGET_TYPE_SECONDARY, &this->SecondaryTargets);
  CHECK_ERROR(zcGetNumTargetsByType, error);

  // Grab a handle to the stylus target.
  error = zcGetTargetByType(this->ZSpaceContext, ZC_TARGET_TYPE_PRIMARY, 0, &this->StylusHandle);
  CHECK_ERROR(zcGetTargetByType, error);

  // Find the zSpace display and set the window's position
  // to be the top left corner of the zSpace display.
  error = zcGetDisplayByType(this->ZSpaceContext, ZC_DISPLAY_TYPE_ZSPACE, 0, &this->DisplayHandle);
  CHECK_ERROR(zcGetDisplayByType, error);

  error = zcGetDisplayPosition(this->DisplayHandle, &this->WindowX, &this->WindowY);
  CHECK_ERROR(zcGetDisplayPosition, error);

  error =
    zcGetDisplayNativeResolution(this->DisplayHandle, &this->WindowWidth, &this->WindowHeight);
  CHECK_ERROR(zcGetDisplayNativeResolution, error);
}

void vtkPVZSpaceView::UpdateTrackers()
{
  // update
  ZCError error;

  vtkRenderWindow* renWin = this->GetRenderWindow();
  int* position = renWin->GetPosition();
  int* size = renWin->GetSize();

  // Update the zSpace viewport position and size based
  // on the position and size of the application window.
  error = zcSetViewportPosition(this->ViewportHandle, position[0], position[1]);
  CHECK_ERROR(zcSetViewportPosition, error);
  error = zcSetViewportSize(this->ViewportHandle, size[0], size[1]);
  CHECK_ERROR(zcSetViewportSize, error);

  // Update the zSpace SDK. This updates both tracking information
  // as well as the head poses for any frustums that have been created.
  error = zcUpdate(this->ZSpaceContext);
  CHECK_ERROR(zcUpdate, error);

  ZCTrackerPose headPose;
  zcGetFrustumHeadPose(this->FrustumHandle, &headPose);

  error = zcTransformMatrix(this->ViewportHandle, ZC_COORDINATE_SPACE_TRACKER,
    ZC_COORDINATE_SPACE_CAMERA, &headPose.matrix);
  CHECK_ERROR(zcTransformMatrix, error);

  // prepare draw
  zcBeginStereoBufferFrame(this->BufferHandle);
}

void vtkPVZSpaceView::UpdateCamera()
{
  vtkCamera* activeCamera = this->GetActiveCamera();

  ZCEye eye = ZC_EYE_CENTER;
  if (activeCamera->GetStereo())
  {
    eye = activeCamera->GetLeftEye() ? ZC_EYE_LEFT : ZC_EYE_RIGHT;
  }

  // get zSpace matrix
  ZSMatrix4 zViewMatrix;
  zcGetFrustumViewMatrix(this->FrustumHandle, eye, &zViewMatrix);

  vtkNew<vtkMatrix4x4> zSpaceViewMatrix;
  for (int i = 0; i < 16; i++)
  {
    zSpaceViewMatrix->SetElement(i % 4, i / 4, zViewMatrix.f[i]);
  }

  // Give to the active camera the zSpace view transform
  vtkNew<vtkTransform> zSpaceViewTransform;
  zSpaceViewTransform->SetMatrix(zSpaceViewMatrix);
  activeCamera->SetUserViewTransform(zSpaceViewTransform);

  ZSMatrix4 zProjectionMatrix;
  zcGetFrustumProjectionMatrix(this->FrustumHandle, eye, &zProjectionMatrix);

  vtkNew<vtkMatrix4x4> zSpaceTransformMatrix;
  for (int i = 0; i < 16; i++)
  {
    zSpaceTransformMatrix->SetElement(i % 4, i / 4, zProjectionMatrix.f[i]);
  }

  // Give to the camera the zSpace projection matrix
  activeCamera->SetExplicitProjectionTransformMatrix(zSpaceTransformMatrix);
  activeCamera->SetUseExplicitProjectionTransformMatrix(true);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::UpdateZSpaceFrustumParameters(bool setPosition)
{
  if (this->GeometryBounds.IsValid() && !this->LockBounds && this->DiscreteCameras == NULL)
  {
    double bounds[6];
    this->GeometryBounds.GetBounds(bounds);
    // Find parameters computed by zSpace SDK
    this->CalculateFit(bounds, setPosition);
  }
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::CalculateFit(double* bounds, bool setPosition)
{
  ZCBoundingBox b;
  b.lower.x = bounds[0];
  b.lower.y = bounds[2];
  b.lower.z = bounds[4];

  b.upper.x = bounds[1];
  b.upper.y = bounds[3];
  b.upper.z = bounds[5];

  ZSMatrix4 lookAtMatrix;
  ZSFloat viewerScale;

  // Calculate the appropriate viewer scale and camera lookat matrix
  // such that content in the above bounding box will take up the entire
  // viewport without being clipped.
  ZCError error = zcCalculateFrustumFit(this->FrustumHandle, &b, &viewerScale, &lookAtMatrix);
  CHECK_ERROR(zcCalculateFrustumFit, error);

  if (setPosition)
  {
    // Get the best position of the camera to observe the bounding box
    this->GetActiveCamera()->SetPosition(-lookAtMatrix.m03, -lookAtMatrix.m13, -lookAtMatrix.m23);
    this->GetActiveCamera()->SetViewUp(lookAtMatrix.m01, lookAtMatrix.m11, lookAtMatrix.m21);
  }

  // Set the frustum's viewer scale with the value that was calculated
  // by zcCalculateFrustumFit().
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_VIEWER_SCALE, viewerScale);

  // Set the near and far clip depending on the vtk clipping range and the viewer scale
  double clippingRange[2];
  this->GetActiveCamera()->GetClippingRange(clippingRange);

  float nearPlane = clippingRange[0] / viewerScale;
  nearPlane = 0.5 * nearPlane;

  float farPlane = clippingRange[1] / viewerScale;
  farPlane = 1.5 * farPlane;

  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_NEAR_CLIP, nearPlane);
  zcSetFrustumAttributeF32(this->FrustumHandle, ZC_FRUSTUM_ATTRIBUTE_FAR_CLIP, farPlane);

  // Check every actors. In case of a surface with edges representation,
  // modify the unit of the offset of edges depending on the viewer scale.
  // Depending on the Z orientation of the camera, the unit is positive or negative
  double* orientation = this->GetActiveCamera()->GetOrientation();
  double sign = orientation[2] < 0 ? -1 : 1;

  vtkActor* a;
  vtkActorCollection* ac = this->GetRenderer()->GetActors();
  vtkCollectionSimpleIterator ait;
  for (ac->InitTraversal(ait); (a = ac->GetNextActor(ait));)
  {
    if (a->GetProperty()->GetRepresentation() == VTK_SURFACE)
    {
      a->GetMapper()->SetResolveCoincidentTopologyLineOffsetParameters(0, sign * 4 / viewerScale);
    }
  }
}

void vtkPVZSpaceView::Render(bool interactive, bool skip_rendering)
{
  this->UpdateTrackers();
  this->UpdateCamera();

  this->Superclass::Render(interactive, skip_rendering);
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "WindowX: " << this->WindowX << "\n";
  os << indent << "WindowY: " << this->WindowY << "\n";
  os << indent << "WindowWidth: " << this->WindowWidth << "\n";
  os << indent << "WindowHeight: " << this->WindowHeight << "\n";
  os << indent << "NbDisplays: " << this->Displays.size() << "\n";
  for (auto display : this->Displays)
  {
    os << indent << "\t" << display << "\n";
  }
  os << indent << "StylusTargets: " << this->StylusTargets << "\n";
  os << indent << "HeadTargets: " << this->HeadTargets << "\n";
  os << indent << "SecondaryTargets: " << this->SecondaryTargets << "\n";
}
