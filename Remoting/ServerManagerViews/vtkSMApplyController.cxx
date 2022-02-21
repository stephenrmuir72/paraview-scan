/*=========================================================================

  Program:   ParaView
  Module:    vtkSMApplyController.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMApplyController.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"

#include <set>

class vtkSMApplyController::vtkInternals
{
};

vtkObjectFactoryNewMacro(vtkSMApplyController);
//----------------------------------------------------------------------------
vtkSMApplyController::vtkSMApplyController() {}

//----------------------------------------------------------------------------
vtkSMApplyController::~vtkSMApplyController() {}

//----------------------------------------------------------------------------
void vtkSMApplyController::MarkShowOnApply(vtkSMProxy* proxy)
{
  if (proxy)
  {
    proxy->SetAnnotation("SHOW_ON_APPLY", "TRUE");
  }
}

//----------------------------------------------------------------------------
void vtkSMApplyController::UnmarkShowOnApply(vtkSMProxy* proxy)
{
  if (proxy)
  {
    proxy->RemoveAnnotation("SHOW_ON_APPLY");
  }
}

//----------------------------------------------------------------------------
bool vtkSMApplyController::IsMarkedShowOnApply(vtkSMProxy* proxy)
{
  return (proxy && proxy->HasAnnotation("SHOW_ON_APPLY") &&
    strcmp(proxy->GetAnnotation("SHOW_ON_APPLY"), "TRUE") == 0);
}

//----------------------------------------------------------------------------
void vtkSMApplyController::Apply(vtkSMSessionProxyManager* pxm)
{
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(pxm);
  iter->SetModeToOneGroup();
  for (iter->Begin("sources"); !iter->IsAtEnd(); iter->Next())
  {
    auto* producer = vtkSMSourceProxy::SafeDownCast(iter->GetProxy());
    if (!vtkSMApplyController::IsMarkedShowOnApply(producer))
    {
      continue;
    }

    auto observable = producer->UpdatePipeline(/**FIXME:**/ 0.0);
    observable.subscribe([producer](bool status) {
      // When this is called, all data information is also up-to-date.
      // So we can now check which representation to create using that data information
      // and also initialize the representation properties that depend on input data information.
      if (status)
      {
        vtkNew<vtkSMApplyController> controller;
        vtkSMApplyController::UnmarkShowOnApply(producer);
        controller->Show(producer);
      }
    });
  }

  // Update all views.
  for (iter->Begin("views"); !iter->IsAtEnd(); iter->Next())
  {
    if (auto* view = vtkSMViewProxy::SafeDownCast(iter->GetProxy()))
    {
      view->InvokeCommand("Update");
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMApplyController::Show(vtkSMSourceProxy* producer)
{
  auto* pxm = producer->GetSessionProxyManager();
  auto* activeView = this->GetActiveView(pxm);
  auto* activeLayout = activeView ? vtkSMViewLayoutProxy::FindLayout(activeView) : nullptr;

  std::vector<vtkSmartPointer<vtkSMSourceProxy>> sources;
  std::set<vtkSMViewProxy*> views;
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  for (int cc = 0, max = producer->GetNumberOfOutputPorts(); cc < max; ++cc)
  {
    auto* preferredView =
      vtkSMViewProxy::SafeDownCast(controller->ShowInPreferredView(producer, cc, activeView));
    if (!preferredView)
    {
      continue;
    }

    // Assign view to layout. This has no effect if already assigned to a
    // layout.
    controller->AssignViewToLayout(preferredView, activeLayout);

    // If a new layout was created, let's use that from now on.
    activeLayout = vtkSMViewLayoutProxy::FindLayout(preferredView);

    // If the source if a filter, we hide the inputs (unless explicitly told
    // otherwise).
    this->HideInputs(producer, preferredView);

    views.insert(preferredView);
  }

  // Handle "reset camera" for each of the views.
  for (auto& view : views)
  {
    this->ResetCamera(view);
  }
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMApplyController::GetActiveView(vtkSMSessionProxyManager* pxm) const
{
  auto* selModel = pxm->GetSelectionModel("ActiveView");
  return selModel ? vtkSMViewProxy::SafeDownCast(selModel->GetCurrentProxy()) : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMApplyController::HideInputs(vtkSMProxy* producer, vtkSMViewProxy* view)
{
  // TODO:
}

//----------------------------------------------------------------------------
void vtkSMApplyController::ResetCamera(vtkSMViewProxy* view)
{
  // TODO:
  view->InvokeCommand("Update");
}

//----------------------------------------------------------------------------
void vtkSMApplyController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
