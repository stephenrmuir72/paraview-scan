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
#include "vtkPVArrayInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"

#include <set>

namespace
{
/**
 * "Replace input" is a hint given to the GUI to turn off input visibility
 * when the filter is created.
 * Returns if this proxy replaces input on creation.
 * This checks the "Hints" for the proxy, if any. If a `<Visibility>` element
 * is present with `replace_input`. It returns its value.
 * Acceptable values are
 * - 0 : Do not hide the input
 * - 1 : Hide the input
 * - 2 : Conditionally turn off the input. The input should be turned
 *        off if the representation is surface and the opacity is 1.
 */
int ShouldHideInput(vtkSMProxy* producer)
{
  vtkPVXMLElement* hints = producer->GetHints();
  if (!hints)
  {
    return 1;
  }
  for (unsigned int cc = 0; cc < hints->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (!child || !child->GetName() || strcmp(child->GetName(), "Visibility") != 0)
    {
      continue;
    }
    int replaceInput = 1;
    if (!child->GetScalarAttribute("replace_input", &replaceInput))
    {
      continue;
    }
    return replaceInput;
  }
  return 1; // default value.
}

}

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
    if (!vtkSMApplyController::IsMarkedShowOnApply(producer) ||
      producer->HasAnnotation("UNDER_INITIALIZATION"))
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
      view->Update();
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

    // If the source is a filter, we hide the inputs (unless explicitly told
    // otherwise).
    this->HideInputs(producer, preferredView);

    // Set some default coloring
    this->InitializeScalarColoring(producer, cc, preferredView);

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
  if (producer->GetProperty("Input") == nullptr)
  {
    // if producer is not a filter, nothing to do.
    return;
  }

  int replaceInput = ShouldHideInput(producer);
  if (replaceInput == 0)
  {
    return;
  }

  vtkSMPropertyHelper inputHelper(producer, "Input", true);
  for (int cc = 0; cc < producer->GetNumberOfProducers(); cc++)
  {
    vtkSMProxy* inputRepr = view->FindRepresentation(
      vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy(cc)), inputHelper.GetOutputPort(cc));
    if (inputRepr == nullptr)
    {
      // if producer's input has no representation in the view, nothing to do.
      return;
    }

    if (replaceInput == 2)
    {
      // Conditionally turn off the input. The input should be turned
      // off if the representation is surface and the opacity is 1.
      std::string reprType = vtkSMPropertyHelper(inputRepr, "Representation").GetAsString();

      double opacity = vtkSMPropertyHelper(inputRepr, "Opacity").GetAsDouble();
      if ((reprType != "Surface" && reprType != "Surface With Edges") ||
        (opacity != 0.0 && opacity < 1.0))
      {
        return;
      }
    }
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    controller->Hide(inputRepr, view);
  }
}

//----------------------------------------------------------------------------
void vtkSMApplyController::ResetCamera(vtkSMViewProxy* view)
{
  view->Update().subscribe([view](bool) { view->ResetCameraUsingVisiblePropBounds(); });
}

//----------------------------------------------------------------------------
void vtkSMApplyController::ApplyScalarColoring(vtkSMProxy* repr)
{
  const bool mapScalars = repr->GetProperty("MapScalars")
    ? vtkSMPropertyHelper(repr, "MapScalars").GetAsInt() == 1
    : true;
  if (!mapScalars)
  {
    return;
  }

  vtkSMPropertyHelper colorArray(repr, "ColorArrayName");
  if (colorArray.GetInputArrayNameToProcess() == nullptr)
  {
    return;
  }

  auto* lut = vtkSMPropertyHelper(repr, "LookupTable").GetAsProxy();
  if (!lut)
  {
    return;
  }

  // reset range is requested by settings.
  auto* domain = colorArray.GetProperty()->FindDomain<vtkSMArrayListDomain>();
  if (!domain)
  {
    return;
  }

  int component = vtkSMPropertyHelper(lut, "VectorMode").GetAsInt() != 0
    ? vtkSMPropertyHelper(lut, "VectorComponent").GetAsInt()
    : -1;

  auto* arrayInfo = domain->GetArrayInformation(
    colorArray.GetInputArrayAssociation(), colorArray.GetInputArrayNameToProcess());
  if (!arrayInfo || component >= arrayInfo->GetNumberOfComponents())
  {
    return;
  }

  double range[2];
  arrayInfo->GetComponentFiniteRange(component, range);
  vtkSMCoreUtilities::AdjustRange(range);
  vtkSMTransferFunctionProxy::RescaleTransferFunction(lut, range, /*extend=*/true);
  lut->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMApplyController::InitializeScalarColoring(
  vtkSMProxy* producer, unsigned int outputPort, vtkSMViewProxy* view)
{
  vtkSMProxy* repr = view->FindRepresentation(vtkSMSourceProxy::SafeDownCast(producer), outputPort);
  if (repr == nullptr)
  {
    // if producer's input has no representation in the view, nothing to do.
    return;
  }

  vtkSMPropertyHelper colorArrayHelper(repr, "ColorArrayName");
  if (colorArrayHelper.GetInputArrayNameToProcess() != nullptr &&
    colorArrayHelper.GetInputArrayNameToProcess()[0] != '\0')
  {
    // a color array is already set
    return;
  }

  auto* colorArrayProperty = colorArrayHelper.GetProperty();
  auto* domain = colorArrayProperty->FindDomain<vtkSMArrayListDomain>();
  int association = 0;
  const char* arrayName = nullptr;
  int numberOfCandidateArrays = 0;
  for (unsigned int idx = 0; idx < domain->GetNumberOfStrings(); idx++)
  {
    association = domain->GetFieldAssociation(idx);
    arrayName = domain->GetString(idx);
    if (association != vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      continue;
    }
    auto* arrayInfo = domain->GetArrayInformation(association, arrayName);
    if (arrayInfo && arrayInfo->GetNumberOfComponents() == 1)
    {
      numberOfCandidateArrays++;
    }
  }

  if (numberOfCandidateArrays != 1)
  {
    return;
  }
  colorArrayHelper.SetInputArrayToProcess(association, arrayName);

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMSessionProxyManager* pxm = producer->GetSessionProxyManager();
  vtkSMProxy* lut = mgr->GetColorTransferFunction(arrayName, pxm);
  vtkSMPropertyHelper(repr, "LookupTable").Set(lut);

  this->ApplyScalarColoring(repr);
}

//----------------------------------------------------------------------------
void vtkSMApplyController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
