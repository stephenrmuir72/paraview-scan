// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRepositionablePropertyWidget_h
#define pqRepositionablePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

#include <vector>

class vtkVector3d;
class vtkRenderer;
class vtkSMRenderViewProxy;

/**
 * Base class for any widgets that can have the <RepositionToView /> proxy hint.
 * This class handles the conversion of display space coordinates into world space coordinates in
 * order to reposition some points relative to the camera position of the render view.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqRepositionablePropertyWidget
  : public pqInteractivePropertyWidget
{
  Q_OBJECT;
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqRepositionablePropertyWidget(const char* widget_smgroup, const char* widget_smname,
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqRepositionablePropertyWidget() override;

protected:
  /**
   * Convert the given display space coordinates into world space coordinates.
   * If there is no renderer or render view proxy, it returns an empty vector.
   */
  std::vector<vtkVector3d> displayToWorldCoordinates(
    const std::vector<vtkVector3d>& displayCoordPoints);

  /**
   * Return true if the RepositionToView hint has been added to the proxy hints.
   */
  bool getHasRepositionToViewHint() const { return this->HasRepositionToViewHint; }

  /**
   * Return the Z coordinate of the focal point in display space (i.e. the depth of the focal
   * point in display space).
   * If there is no renderer or render view proxy, it returns -1.
   */
  double getFocalPointDepth();

  /**
   * Return the renderer object from the current active view.
   * If the renderer does not exists, it returns a nullptr.
   */
  vtkRenderer* getRenderer();

private:
  Q_DISABLE_COPY(pqRepositionablePropertyWidget)

  /**
   * Return the current active view.
   * If the render view does not exists, it returns a nullptr.
   */
  vtkSMRenderViewProxy* getRenderViewProxy();

  bool HasRepositionToViewHint = false;
};

#endif // pqRepositionablePropertyWidget_h
