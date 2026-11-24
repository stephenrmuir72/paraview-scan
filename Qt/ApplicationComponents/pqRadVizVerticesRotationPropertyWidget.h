// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqRadVizVerticesRotationPropertyWidget_h
#define pqRadVizVerticesRotationPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqDoubleVectorPropertyWidget.h"
#include "vtkNew.h" // For vtkNew

class vtkEventQtSlotConnect;

/**
 * pqRadVizVerticesRotationPropertyWidget is the widget for the RadViz
 * representation's VerticesRotation property (panel_widget
 * "radviz_vertices_rotation"). It is a pqDoubleVectorPropertyWidget that
 * also rotates the representation's LensCenter around the origin by the
 * change the user makes to the rotation, as
 * vtkPlotRadViz::SetVerticesAndLensRotation does, so the lens keeps the same
 * data points and only the colors change. Like the MultiVariateToRGBA
 * filter's editor (pqMultiVariateColorEditorWidget), it rotates the lens only
 * for edits made in this widget: rotations set by Python, a state file or
 * the sync with the filter leave the lens as set, and a drag in the RadViz
 * view sets both the rotation and the rotated lens itself.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqRadVizVerticesRotationPropertyWidget
  : public pqDoubleVectorPropertyWidget
{
  Q_OBJECT
  typedef pqDoubleVectorPropertyWidget Superclass;

public:
  pqRadVizVerticesRotationPropertyWidget(
    vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = nullptr);
  ~pqRadVizVerticesRotationPropertyWidget() override;

  ///@{
  /**
   * Also apply, or reset, the LensCenter rotated by rotateLens(): the Display
   * panel applies only the widget that changed, and LensCenter has its own.
   */
  void apply() override;
  void reset() override;
  ///@}

private Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Records the rotation before and after each change of the property's
   * unchecked (pending Apply) value, whatever made it.
   */
  void rotationModified();

  /**
   * Rotates LensCenter by the last change of the rotation. Called on
   * changeAvailable(), which the property link emits only for edits in this
   * widget, after it stores the new rotation.
   */
  void rotateLens();

private:
  Q_DISABLE_COPY(pqRadVizVerticesRotationPropertyWidget)

  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  double PreviousRotation = 0.0;
  double Rotation = 0.0;
};

#endif
