/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.cxx

  Copyright (c) Kitware, Inc.
  Copyright (c) 2017, NVIDIA CORPORATION.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderView.h"

#include "vtkCamera.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

class vtkPVRenderView::vtkInternals
{
public:
  vtkSmartPointer<vtkRenderWindow> RenderWindow;
  vtkSmartPointer<vtkRenderer> Renderers[2];
  vtkSmartPointer<vtkCamera> Camera;
  vtkSmartPointer<vtkPVAxesWidget> OrientationWidget;
  vtkSmartPointer<vtkPVSynchronizedRenderer> SynchronizedRenderers;
};

vtkStandardNewMacro(vtkPVRenderView);
//-----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
  : Internals(new vtkPVRenderView::vtkInternals())
{
}

//-----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView() = default;

//-----------------------------------------------------------------------------
void vtkPVRenderView::InitializeForDataProcessing()
{
  this->Superclass::InitializeForDataProcessing();
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::InitializeForRendering()
{
  this->Superclass::InitializeForRendering();

  auto& internals = (*this->Internals);
  internals.RenderWindow = vtk::TakeSmartPointer(this->NewRenderWindow());
  internals.RenderWindow->SetNumberOfLayers(3);
  internals.RenderWindow->SetMultiSamples(0);

  internals.Camera = vtk::TakeSmartPointer(vtkCamera::New());

  // 3D renderer
  internals.Renderers[0] = vtk::TakeSmartPointer(vtkRenderer::New());
  internals.Renderers[0]->SetActiveCamera(internals.Camera);

  // 2D renderer
  internals.Renderers[1] = vtk::TakeSmartPointer(vtkRenderer::New());
  internals.Renderers[1]->SetLayer(2);
  internals.Renderers[1]->EraseOff();
  internals.Renderers[1]->InteractiveOff();
  internals.Renderers[1]->SetActiveCamera(internals.Camera);

  internals.RenderWindow->AddRenderer(internals.Renderers[0]);
  internals.RenderWindow->AddRenderer(internals.Renderers[1]);

  // Setup distributed rendering, if needed.
  auto* controller = this->GetController();
  internals.SynchronizedRenderers = vtk::TakeSmartPointer(vtkPVSynchronizedRenderer::New());
  internals.SynchronizedRenderers->Initialize(controller);
  internals.SynchronizedRenderers->SetRenderer(internals.Renderers[0]);

  // Setup OrientationWidget
  internals.OrientationWidget = vtk::TakeSmartPointer(vtkPVAxesWidget::New());
  internals.OrientationWidget->SetParentRenderer(internals.Renderers[0]);
  internals.OrientationWidget->SetViewport(0, 0, 0.25, 0.25);
}

//-----------------------------------------------------------------------------
vtkRenderer* vtkPVRenderView::GetRenderer(int rendererType /*= DEFAULT_RENDERER*/) const
{
  const auto& internals = (*this->Internals);
  return internals.Renderers[rendererType];
}

//-----------------------------------------------------------------------------
vtkRenderWindow* vtkPVRenderView::GetRenderWindow() const
{
  const auto& internals = (*this->Internals);
  return internals.RenderWindow;
}

//-----------------------------------------------------------------------------
vtkCamera* vtkPVRenderView::GetActiveCamera() const
{
  const auto& internals = (*this->Internals);
  return internals.Camera;
}

//-----------------------------------------------------------------------------
vtkBoundingBox vtkPVRenderView::GetVisiblePropBounds()
{
  if (auto* renderer = this->GetRenderer())
  {
    double bounds[6];
    renderer->ComputeVisiblePropBounds(bounds);

    vtkBoundingBox bbox;
    bbox.SetBounds(bounds);

    auto* controller = this->GetController();
    controller->Reduce(bbox, bbox, 0);
    if (controller->GetLocalProcessId() == 0)
    {
      return bbox;
    }
  }

  return vtkBoundingBox();
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
