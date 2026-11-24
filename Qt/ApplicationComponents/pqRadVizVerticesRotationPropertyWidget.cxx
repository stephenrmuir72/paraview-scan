// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqRadVizVerticesRotationPropertyWidget.h"

#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMultiVariateToRGBA.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <array>

namespace
{
const char* LENS_CENTER_PROPERTY = "LensCenter";
}

//-----------------------------------------------------------------------------
pqRadVizVerticesRotationPropertyWidget::pqRadVizVerticesRotationPropertyWidget(
  vtkSMProperty* smProperty, vtkSMProxy* smProxy, QWidget* parentObject)
  : Superclass(smProperty, smProxy, parentObject)
{
  this->Rotation = vtkSMUncheckedPropertyHelper(smProperty).GetAsDouble();
  this->PreviousRotation = this->Rotation;
  // also fired when the checked value changes, as it resets the unchecked one
  this->VTKConnector->Connect(
    smProperty, vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(rotationModified()));
  QObject::connect(this, &pqPropertyWidget::changeAvailable, this,
    &pqRadVizVerticesRotationPropertyWidget::rotateLens);
}

//-----------------------------------------------------------------------------
pqRadVizVerticesRotationPropertyWidget::~pqRadVizVerticesRotationPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqRadVizVerticesRotationPropertyWidget::apply()
{
  BEGIN_UNDO_SET(tr("Property Changed"));
  vtkSMProxy* smProxy = this->proxy();
  vtkSMUncheckedPropertyHelper uncheckedLens(smProxy, ::LENS_CENTER_PROPERTY);
  double lensCenter[2];
  uncheckedLens.Get(lensCenter, 2);
  vtkSMPropertyHelper(smProxy, ::LENS_CENTER_PROPERTY).Set(lensCenter, 2);
  this->Superclass::apply();
  smProxy->UpdateVTKObjects();
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqRadVizVerticesRotationPropertyWidget::reset()
{
  this->proxy()->GetProperty(::LENS_CENTER_PROPERTY)->ClearUncheckedElements();
  this->Superclass::reset();
}

//-----------------------------------------------------------------------------
void pqRadVizVerticesRotationPropertyWidget::rotationModified()
{
  const double rotation = vtkSMUncheckedPropertyHelper(this->property()).GetAsDouble();
  if (rotation != this->Rotation)
  {
    this->PreviousRotation = this->Rotation;
    this->Rotation = rotation;
  }
}

//-----------------------------------------------------------------------------
void pqRadVizVerticesRotationPropertyWidget::rotateLens()
{
  const double angle = this->Rotation - this->PreviousRotation;
  // a later changeAvailable() without a new rotation must not rotate again
  this->PreviousRotation = this->Rotation;
  if (angle == 0)
  {
    return;
  }
  vtkSMUncheckedPropertyHelper lensHelper(this->proxy(), ::LENS_CENTER_PROPERTY);
  double lensCenter[2];
  lensHelper.Get(lensCenter, 2);
  std::array<double, 2> rotated = vtkMultiVariateToRGBA::RotateLensCenter(lensCenter, angle);
  lensHelper.Set(rotated.data(), 2);
}
