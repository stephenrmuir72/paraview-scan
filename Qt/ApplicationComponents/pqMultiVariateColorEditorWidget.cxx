// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqMultiVariateColorEditorWidget.h"
#include "ui_pqMultiVariateColorEditorWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"
#include "pqProxy.h"
#include "pqRadVizChartView.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkMultiVariateToRGBA.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMViewProxy.h"
#include "vtkType.h"

#include <QDoubleValidator>
#include <QPointer>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>

namespace
{
const char* VERTICES_ROTATION_PROPERTY = "VerticesRotation";
const char* LENS_CENTER_PROPERTY = "LensCenter";
const char* LENS_RADIUS_PROPERTY = "LensRadius";
const char* INVERT_LENS_PROPERTY = "InvertLens";
const char* SAMPLING_DIMENSIONS_PROPERTY = "SamplingDimensions";
// Number of samples along the longest side of the input's bounds.
const int RESAMPLE_MAX_SAMPLING_DIMENSION = 10;

// Computes sampling dimensions proportional to the sides of 'bounds', so
// the samples are evenly spaced in all directions, with
// RESAMPLE_MAX_SAMPLING_DIMENSION samples along the longest side. A side
// with no length gets a single sample. If 'bounds' are not valid, uses
// RESAMPLE_MAX_SAMPLING_DIMENSION in all directions.
void computeSamplingDimensions(const double bounds[6], int dimensions[3])
{
  double lengths[3];
  double maxLength = 0.0;
  for (int i = 0; i < 3; ++i)
  {
    lengths[i] = bounds[2 * i + 1] - bounds[2 * i];
    maxLength = std::max(maxLength, lengths[i]);
  }
  for (int i = 0; i < 3; ++i)
  {
    if (maxLength <= 0.0)
    {
      dimensions[i] = ::RESAMPLE_MAX_SAMPLING_DIMENSION;
    }
    else if (lengths[i] <= 0.0)
    {
      dimensions[i] = 1;
    }
    else
    {
      // at least 2 samples, to cover both ends of the side.
      dimensions[i] = std::max(2,
        static_cast<int>(std::lround(::RESAMPLE_MAX_SAMPLING_DIMENSION * lengths[i] / maxLength)));
    }
  }
}
}

class pqMultiVariateColorEditorWidget::pqInternal : public Ui::MultiVariateColorEditorWidget
{
public:
  QPointer<pqRadVizChartView> RadVizView;
  // ResampleToImage filter that downsamples the input before it is
  // downloaded to the client to feed the RadViz representation.
  QPointer<pqPipelineSource> ResampleFilter;
  // RadViz representation of the (resampled) input, shown in RadVizView.
  QPointer<pqDataRepresentation> RadVizRepresentation;
  pqInternal(pqMultiVariateColorEditorWidget* self) { this->setupUi(self); }
};

pqMultiVariateColorEditorWidget::pqMultiVariateColorEditorWidget(
  vtkSMProxy* smProxy, vtkSMPropertyGroup* smGroup, QWidget* parentObject)
  : Superclass(smProxy, smGroup, parentObject)
  , Internal(new pqInternal(this))
{
  Ui::MultiVariateColorEditorWidget& ui = *this->Internal;
  ui.mainLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin());
  ui.mainLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  ui.mainLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

  ui.VerticesRotation->setValidator(new QDoubleValidator(0, 359.999, 6, ui.VerticesRotation));
  ui.LensCenterX->setValidator(new QDoubleValidator(ui.LensCenterX));
  ui.LensCenterY->setValidator(new QDoubleValidator(ui.LensCenterY));
  ui.LensRadius->setValidator(new QDoubleValidator(0, VTK_DOUBLE_MAX, 6, ui.LensRadius));

  // connected before the VerticesRotation link, so the property still holds the old rotation
  QObject::connect(ui.VerticesRotation, &QLineEdit::editingFinished, this,
    &pqMultiVariateColorEditorWidget::rotateLensWithVertices);
  // edit controls change -> proxy properties change
  this->addPropertyLink(ui.VerticesRotation, ::VERTICES_ROTATION_PROPERTY);
  this->addPropertyLink(ui.LensCenterX, ::LENS_CENTER_PROPERTY, 0);
  this->addPropertyLink(ui.LensCenterY, ::LENS_CENTER_PROPERTY, 1);
  this->addPropertyLink(ui.LensRadius, ::LENS_RADIUS_PROPERTY);
  this->addPropertyLink(ui.InvertLens, ::INVERT_LENS_PROPERTY);

  QObject::connect(
    ui.ShowRadViz, &QPushButton::clicked, this, &pqMultiVariateColorEditorWidget::showRadViz);

  // stop tracking the RadViz objects when the user deletes them.
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smModel, &pqServerManagerModel::sourceRemoved, this,
    &pqMultiVariateColorEditorWidget::sourceRemoved);
  QObject::connect(smModel, &pqServerManagerModel::representationRemoved, this,
    &pqMultiVariateColorEditorWidget::representationRemoved);
  QObject::connect(smModel, &pqServerManagerModel::viewRemoved, this,
    &pqMultiVariateColorEditorWidget::viewRemoved);

  // reuse a RadViz representation already set up for this filter.
  this->findExistingRadViz();
}

pqMultiVariateColorEditorWidget::~pqMultiVariateColorEditorWidget()
{
  // only stops tracking the RadViz objects, does not delete them.
  this->forgetRadViz();
  delete this->Internal;
  this->Internal = nullptr;
}

void pqMultiVariateColorEditorWidget::findExistingRadViz()
{
  vtkSMProxy* filterProxy = this->proxy();
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  // a RadViz view shows one representation at a time: prefer the visible one.
  pqDataRepresentation* found = nullptr;
  for (pqDataRepresentation* repr : smModel->findItems<pqDataRepresentation*>())
  {
    vtkSMProxy* radVizProxy = repr->getProxy();
    // skip a representation not (yet) in a RadViz view, e.g. during a state load.
    if (vtkSMPropertyHelper(radVizProxy, pqRadVizChartView::RADVIZ_SOURCE_PROPERTY, /*quiet=*/true)
          .GetAsProxy() == filterProxy &&
      qobject_cast<pqRadVizChartView*>(repr->getView()) && (!found || !found->isVisible()))
    {
      found = repr;
    }
  }
  if (!found)
  {
    return;
  }

  // the input is a ResampleToImage filter or this filter's input.
  pqPipelineSource* reprInput = found->getInput();
  vtkSMProxy* reprInputProxy = reprInput ? reprInput->getProxy() : nullptr;
  bool resampled = reprInputProxy && reprInputProxy->GetXMLName() &&
    strcmp(reprInputProxy->GetXMLName(), "ResampleToImage") == 0;

  this->Internal->RadVizRepresentation = found;
  this->Internal->ResampleFilter = resampled ? reprInput : nullptr;
  this->Internal->RadVizView = qobject_cast<pqRadVizChartView*>(found->getView());
  this->connectRadVizView();
  this->Internal->Downsample->setChecked(resampled);

  // the name may already carry the annotation (state reload, filter renamed). The
  // view is named after the filter it shows.
  if (found->isVisible())
  {
    this->updateRadVizViewAnnotation();
  }

  // the view owns the property sync; make sure it is established, e.g. after a
  // state reload where the view saw the annotation late.
  this->Internal->RadVizView->updateConnection();
}

void pqMultiVariateColorEditorWidget::forgetRadViz()
{
  if (this->Internal->RadVizView)
  {
    QObject::disconnect(this->Internal->RadVizView, nullptr, this, nullptr);
  }
  this->Internal->RadVizRepresentation = nullptr;
  this->Internal->RadVizView = nullptr;
  this->Internal->ResampleFilter = nullptr;
}

void pqMultiVariateColorEditorWidget::connectRadVizView()
{
  if (!this->Internal->RadVizView)
  {
    return;
  }
  QObject::connect(this->Internal->RadVizView, &pqRadVizChartView::filterPropertiesStagedFromView,
    this,
    [this]()
    {
      // the view stages the filter of its visible representation.
      if (this->Internal->RadVizRepresentation && this->Internal->RadVizRepresentation->isVisible())
      {
        Q_EMIT this->changeAvailable();
        Q_EMIT this->changeFinished();
      }
    });
  // a disconnected view no longer belongs to this filter.
  QObject::connect(this->Internal->RadVizView, &pqRadVizChartView::disconnectedFromFilter, this,
    &pqMultiVariateColorEditorWidget::forgetRadViz);
}

void pqMultiVariateColorEditorWidget::rotateLensWithVertices()
{
  bool ok = false;
  const double rotation = this->Internal->VerticesRotation->text().toDouble(&ok);
  if (!ok)
  {
    return;
  }
  // the panel edits the unchecked (pending Apply) values, as a drag in the RadViz view does
  vtkSMProxy* filterProxy = this->proxy();
  vtkSMUncheckedPropertyHelper rotationHelper(filterProxy, ::VERTICES_ROTATION_PROPERTY);
  const double oldRotation = rotationHelper.GetAsDouble();
  vtkSMUncheckedPropertyHelper lensHelper(filterProxy, ::LENS_CENTER_PROPERTY);
  double lensCenter[2];
  lensHelper.Get(lensCenter, 2);
  std::array<double, 2> rotated =
    vtkMultiVariateToRGBA::RotateLensCenter(lensCenter, rotation - oldRotation);
  // unchanged for an unchanged rotation (cos 0 = 1, sin 0 = 0), so nothing is staged
  lensHelper.Set(rotated.data(), 2);
}

void pqMultiVariateColorEditorWidget::showRadViz()
{
  SCOPED_UNDO_SET(tr("Show RadViz"));
  if (!this->Internal->RadVizView)
  {
    // the representation may have been set up after construction.
    this->findExistingRadViz();
  }

  if (this->Internal->RadVizView)
  {
    // already set up: show it, which hides another filter's, and make its view active.
    pqDataRepresentation* repr = this->Internal->RadVizRepresentation;
    if (repr && !repr->isVisible())
    {
      repr->setVisible(true);
      vtkSMViewProxy::HideOtherRepresentationsIfNeeded(
        this->Internal->RadVizView->getViewProxy(), repr->getProxy());
      this->Internal->RadVizView->render();
    }
    pqActiveObjects::instance().setActiveView(this->Internal->RadVizView);
    return;
  }

  pqPipelineSource* filterSource =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(
      this->proxy());
  if (!filterSource)
  {
    return;
  }

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  // RadViz and this filter both show the filter's input
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(filterSource);
  pqOutputPort* inputPort = filter ? filter->getAnyInput() : nullptr;
  if (!inputPort)
  {
    return;
  }

  pqOutputPort* radVizInputPort;
  if (this->Internal->Downsample->isChecked())
  {
    // downsample the input before it is downloaded to the client. Deleting this
    // filter or closing the RadViz view leaves the ResampleToImage filter in the
    // pipeline on purpose: the user deletes it. It may be used elsewhere, and a
    // RadViz view connected by hand may show a ResampleToImage the user created.
    pqPipelineSource* resampleFilter = builder->createFilter(
      "filters", "ResampleToImage", inputPort->getSource(), inputPort->getPortNumber());
    if (!resampleFilter)
    {
      return;
    }
    this->Internal->ResampleFilter = resampleFilter;
    double bounds[6] = { 0.0, -1.0, 0.0, -1.0, 0.0, -1.0 };
    if (vtkPVDataInformation* inputInfo = inputPort->getDataInformation())
    {
      inputInfo->GetBounds(bounds);
    }
    int samplingDimensions[3];
    ::computeSamplingDimensions(bounds, samplingDimensions);
    vtkSMPropertyHelper(resampleFilter->getProxy(), ::SAMPLING_DIMENSIONS_PROPERTY)
      .Set(samplingDimensions, 3);
    resampleFilter->getProxy()->UpdateVTKObjects();
    // already pushed, so don't highlight Apply.
    resampleFilter->setModifiedState(pqProxy::UNMODIFIED);
    // needed for the representation's default series labels.
    resampleFilter->updatePipeline();

    radVizInputPort = resampleFilter->getOutputPort(0);
  }
  else
  {
    radVizInputPort = inputPort;
  }

  auto* view = qobject_cast<pqRadVizChartView*>(
    builder->createView(pqRadVizChartView::chartViewType(), radVizInputPort->getServer()));
  if (view)
  {
    builder->addToLayout(view);
  }
  pqDataRepresentation* repr = view
    ? builder->createDataRepresentation(radVizInputPort, view, "RadVizRepresentation")
    : nullptr;
  if (!repr)
  {
    // remove what was created for it.
    if (view)
    {
      builder->destroy(view);
    }
    pqPipelineSource* resampleFilter = this->Internal->ResampleFilter;
    this->Internal->ResampleFilter = nullptr;
    if (resampleFilter)
    {
      builder->destroy(resampleFilter);
    }
    return;
  }

  // lets findExistingRadViz() find the representation again.
  vtkSMProxy* radVizProxy = repr->getProxy();
  vtkSMPropertyHelper(radVizProxy, pqRadVizChartView::RADVIZ_SOURCE_PROPERTY).Set(this->proxy());
  radVizProxy->UpdateVTKObjects();

  this->Internal->RadVizRepresentation = repr;
  this->Internal->RadVizView = view;
  this->connectRadVizView();
  this->updateRadVizViewAnnotation();

  this->selectMultiVariateArrayComponents();

  // establish the live sync between the view and this filter: interacting with
  // the view stages the filter's properties and highlights Apply.
  this->Internal->RadVizView->updateConnection();
}

void pqMultiVariateColorEditorWidget::updateRadVizViewAnnotation()
{
  if (!this->Internal->RadVizView)
  {
    return;
  }

  pqPipelineSource* filterSource =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(
      this->proxy());
  if (!filterSource)
  {
    return;
  }

  this->Internal->RadVizView->setFilterAnnotation(filterSource->getSMName());
}

void pqMultiVariateColorEditorWidget::selectMultiVariateArrayComponents()
{
  if (this->Internal->RadVizRepresentation)
  {
    pqRadVizChartView::showFilterComponents(
      this->Internal->RadVizRepresentation->getProxy(), this->proxy());
  }
}

//----------------------------------------------------------------------------
void pqMultiVariateColorEditorWidget::removeRadVizViewAnnotation()
{
  if (this->Internal->RadVizView)
  {
    this->Internal->RadVizView->setFilterAnnotation(QString());
  }
}

void pqMultiVariateColorEditorWidget::sourceRemoved(pqPipelineSource* source)
{
  if (source == this->Internal->ResampleFilter)
  {
    this->forgetRadViz();
  }
  else if (source->getProxy() == this->proxy())
  {
    // the filter is being removed: drop the view's annotation.
    this->removeRadVizViewAnnotation();
    this->forgetRadViz();
  }
}

void pqMultiVariateColorEditorWidget::representationRemoved(pqRepresentation* repr)
{
  if (repr == this->Internal->RadVizRepresentation)
  {
    this->forgetRadViz();
  }
}

void pqMultiVariateColorEditorWidget::viewRemoved(pqView* view)
{
  if (view == this->Internal->RadVizView)
  {
    this->forgetRadViz();
  }
}
