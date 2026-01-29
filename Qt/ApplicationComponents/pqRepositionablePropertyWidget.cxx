// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqRepositionablePropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVector.h"

#include <QWidget>

//-----------------------------------------------------------------------------
pqRepositionablePropertyWidget::pqRepositionablePropertyWidget(const char* widgetSMgroup,
  const char* widgetSMName, vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parent)
  : Superclass(widgetSMgroup, widgetSMName, smproxy, smgroup, parent)
{
  vtkPVXMLElement* hints = smproxy->GetHints();
  if (hints)
  {
    for (unsigned int i = 0; i < hints->GetNumberOfNestedElements(); i++)
    {
      vtkPVXMLElement* hintsElement = hints->GetNestedElement(i);
      if (strcmp(hintsElement->GetName(), "RepositionToView") == 0)
      {
        this->HasRepositionToViewHint = true;
      }
    }
  }
}

//-----------------------------------------------------------------------------
pqRepositionablePropertyWidget::~pqRepositionablePropertyWidget() = default;

//-----------------------------------------------------------------------------
std::vector<vtkVector3d> pqRepositionablePropertyWidget::displayToWorldCoordinates(
  const std::vector<vtkVector3d>& displayCoordPoints)
{
  vtkRenderer* renderer = this->getRenderer();
  if (!renderer)
  {
    return {};
  }

  std::vector<vtkVector3d> worldCoordPoints(displayCoordPoints.size());
  // Compute all points from display coord to world coord.
  for (std::size_t i = 0; i < displayCoordPoints.size(); i++)
  {
    worldCoordPoints[i] = renderer->DisplayToWorld(displayCoordPoints[i]);
  }

  return worldCoordPoints;
}

//-----------------------------------------------------------------------------
double pqRepositionablePropertyWidget::getFocalPointDepth()
{
  vtkSMRenderViewProxy* renderViewProxy = this->getRenderViewProxy();
  vtkRenderer* renderer = this->getRenderer();
  if (!renderViewProxy || !renderer)
  {
    return -1.0;
  }

  // Recover focal point in display coordinates to get the Z coordinate for the new depth position
  // of the ruler.
  double cameraFocalPointWorldCoord[3] = { 0.0, 0.0, 0.0 };
  vtkSMPropertyHelper(renderViewProxy, "CameraFocalPoint").Get(cameraFocalPointWorldCoord, 3);
  double cameraFocalPointDisplayCoord[3] = { 0.0, 0.0, 0.0 };
  renderer->SetWorldPoint(cameraFocalPointWorldCoord[0], cameraFocalPointWorldCoord[1],
    cameraFocalPointWorldCoord[2], 1.0);
  renderer->WorldToDisplay(cameraFocalPointDisplayCoord[0], cameraFocalPointDisplayCoord[1],
    cameraFocalPointDisplayCoord[2]);

  return cameraFocalPointDisplayCoord[2];
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy* pqRepositionablePropertyWidget::getRenderViewProxy()
{
  pqRenderView* activeView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!activeView)
  {
    return nullptr;
  }

  return vtkSMRenderViewProxy::SafeDownCast(activeView->getProxy());
}

//-----------------------------------------------------------------------------
vtkRenderer* pqRepositionablePropertyWidget::getRenderer()
{
  vtkSMRenderViewProxy* renderViewProxy = this->getRenderViewProxy();
  if (!renderViewProxy)
  {
    return nullptr;
  }

  return renderViewProxy->GetRenderer();
}
