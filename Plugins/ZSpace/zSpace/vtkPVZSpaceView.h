/*=========================================================================

  Program:   ParaView
  Module:    vtkPVZSpaceView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVZSpaceView
 *
 * vtkPVZSpaceView extends vtkPVRenderView to render the scene in a zSpace device.
 */

#ifndef vtkPVPanoramicProjectionView_h
#define vtkPVPanoramicProjectionView_h

#include "vtkNew.h" // for vtkNew
#include "vtkPVRenderView.h"
#include "vtkZSpaceViewModule.h" // for export macro

#include "vtkActor.h"
#include "vtkLineSource.h"
#include "vtkVector.h"

#include <zSpace.h>

class VTKZSPACEVIEW_EXPORT vtkPVZSpaceView : public vtkPVRenderView
{
public:
  static vtkPVZSpaceView* New();
  vtkTypeMacro(vtkPVZSpaceView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetMacro(WindowX, int);
  vtkGetMacro(WindowY, int);

  vtkGetMacro(WindowWidth, int);
  vtkGetMacro(WindowHeight, int);

  vtkGetMacro(StylusTargets, int);
  vtkGetMacro(HeadTargets, int);
  vtkGetMacro(SecondaryTargets, int);

  // Compute camera position, view up, focal point and frustum near and far clip
  // From the scene bounding box
  void UpdateZSpaceFrustumParameters(bool setPosition = false);

protected:
  vtkPVZSpaceView();
  ~vtkPVZSpaceView() override;

  void Render(bool interactive, bool skip_rendering) override;

  void InitializeZSpace();
  void UpdateTrackers();

  // Get the view and projection matrix from zSpace and give them
  // to the active camera
  void UpdateCamera();

  // Give to the zSpace SDK the scene bounds to let it choose the best
  // viewer scale, near and far clip. If setPosition is true, the camera position
  // and viewUp is set with the zSpace advised position
  void CalculateFit(double* bounds, bool setPosition = false);

  void ResetCamera();

  // Override ResetCameraClippingRange to call CalculateFit in order to set
  // the zSpace advised viewer scale, near and far clip
  void ResetCameraClippingRange() override;

private:
  vtkPVZSpaceView(const vtkPVZSpaceView&) = delete;
  void operator=(const vtkPVZSpaceView&) = delete;

  ZCContext ZSpaceContext = nullptr;
  ZCHandle DisplayHandle = nullptr;
  ZCHandle BufferHandle = nullptr;
  ZCHandle ViewportHandle = nullptr;
  ZCHandle FrustumHandle = nullptr;
  ZCHandle StylusHandle = nullptr;

  int WindowX = 0;
  int WindowY = 0;

  int WindowWidth = 0;
  int WindowHeight = 0;

  // Store the type for each detected display devices
  std::vector<std::string> Displays;
  // The number of stylus
  int StylusTargets;
  // The number of glasses
  int HeadTargets;
  // Additional targets
  int SecondaryTargets;
};

#endif
