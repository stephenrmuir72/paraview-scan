// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqRadVizChartView.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRadVizRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

const char* const pqRadVizChartView::RADVIZ_SOURCE_PROPERTY = "MultiVariateSource";

namespace
{
// How long the mouse must pause during a drag before the change is staged
// onto the filter.
const int INTERACTION_SYNC_DELAY_MS = 300;

const char* VERTICES_ROTATION_PROPERTY = "VerticesRotation";
const char* VERTICES_CLOCKWISE_ORDERING_PROPERTY = "VerticesClockwiseOrdering";
const char* LENS_CENTER_PROPERTY = "LensCenter";
const char* LENS_RADIUS_PROPERTY = "LensRadius";
const char* PARTITION_INDICES_PROPERTY = "PartitionIndices";
const char* PARTITION_OFFSETS_PROPERTY = "PartitionOffsets";
const char* CUSTOM_RANGE_PROPERTY = "CustomRange";
const char* SYNCED_PROPERTIES[] = { ::VERTICES_ROTATION_PROPERTY,
  ::VERTICES_CLOCKWISE_ORDERING_PROPERTY, ::LENS_CENTER_PROPERTY, ::LENS_RADIUS_PROPERTY,
  ::PARTITION_INDICES_PROPERTY, ::PARTITION_OFFSETS_PROPERTY, ::CUSTOM_RANGE_PROPERTY };
const char* VISIBLE_COMPONENTS_PROPERTY = "VisibleComponents";
// smallest number of visible components, as in pqComponentVisibilityWidget.
const size_t MIN_VISIBLE_COMPONENTS = 3;
const char* SERIES_VISIBILITY_PROPERTY = "SeriesVisibility";
const char* MULTIVARIATE_ARRAY_PROPERTY = "MultiVariateArray";
const char* FIELD_ASSOCIATION_PROPERTY = "FieldAssociation";

// Returns the information of the filter's MultiVariateArray, passed through
// to its output, and sets arrayName, or returns nullptr.
vtkPVArrayInformation* MultiVariateArrayInformation(vtkSMProxy* filterProxy, std::string& arrayName)
{
  // elements: [index, port, connection, fieldAssociation, arrayName].
  vtkSMPropertyHelper arrayHelper(filterProxy, ::MULTIVARIATE_ARRAY_PROPERTY);
  const char* name = arrayHelper.GetAsString(4);
  auto* sourceProxy = vtkSMSourceProxy::SafeDownCast(filterProxy);
  vtkPVDataInformation* dataInfo = sourceProxy ? sourceProxy->GetDataInformation(0) : nullptr;
  vtkPVDataSetAttributesInformation* attrInfo =
    dataInfo ? dataInfo->GetAttributeInformation(arrayHelper.GetAsInt(3)) : nullptr;
  if (!name || !name[0] || !attrInfo)
  {
    return nullptr;
  }
  arrayName = name;
  return attrInfo->GetArrayInformation(name);
}

// Returns the series name of component c of the array arrayName.
std::string ComponentSeriesName(
  const std::string& arrayName, vtkPVArrayInformation* arrayInfo, int c)
{
  return arrayName + "_" + arrayInfo->GetComponentName(c);
}

// Returns the component of the array arrayName that the series seriesName shows,
// or -1. In Individual Blocks mode, series names end with " (blockName)".
int SeriesComponent(
  const std::string& seriesName, const std::string& arrayName, vtkPVArrayInformation* arrayInfo)
{
  for (int c = 0; c < arrayInfo->GetNumberOfComponents(); ++c)
  {
    const std::string name = ::ComponentSeriesName(arrayName, arrayInfo, c);
    if (seriesName == name ||
      (seriesName.size() > name.size() + 3 &&
        seriesName.compare(0, name.size() + 2, name + " (") == 0 && seriesName.back() == ')'))
    {
      return c;
    }
  }
  return -1;
}

// Returns the filter's visible components, as indices in [0, total) in
// increasing order, where total is the number of components of its
// MultiVariateArray: indices outside that range are dropped, and an empty
// VisibleComponents gives all of them, as vtkMultiVariateToRGBA processes all
// components for an empty list (see AddVisibleVariableIndex). Uses the unchecked
// (pending Apply) values if unchecked is true.
std::vector<int> FilterVisibleComponents(vtkSMProxy* filterProxy, int total, bool unchecked)
{
  vtkSMPropertyHelper helper(filterProxy, ::VISIBLE_COMPONENTS_PROPERTY);
  helper.SetUseUnchecked(unchecked);
  std::vector<int> indices;
  for (unsigned int i = 0; i < helper.GetNumberOfElements(); ++i)
  {
    const int index = helper.GetAsInt(i);
    if (index >= 0 && index < total)
    {
      indices.push_back(index);
    }
  }
  std::sort(indices.begin(), indices.end());
  indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
  if (indices.empty())
  {
    for (int i = 0; i < total; ++i)
    {
      indices.push_back(i);
    }
  }
  return indices;
}

// Copies repr's synced properties onto filter's same-named properties, without
// pushing them to the filter's VTK object, and marks the filter modified so that
// Apply gets highlighted. With unchecked, sets the unchecked (pending Apply)
// values, as if typed in the filter's panel: the actual values, which Reset
// restores, are left untouched until Apply.
void CopyRadVizPropertiesToFilter(
  pqDataRepresentation* repr, pqPipelineSource* filter, bool unchecked)
{
  vtkSMProxy* filterProxy = filter->getProxy();
  vtkSMProxy* radVizProxy = repr->getProxy();
  for (const char* propertyName : ::SYNCED_PROPERTIES)
  {
    vtkSMPropertyHelper filterHelper(filterProxy, propertyName);
    filterHelper.SetUseUnchecked(unchecked);
    filterHelper.Copy(vtkSMPropertyHelper(radVizProxy, propertyName));
  }
  filter->setModifiedState(pqProxy::MODIFIED);
}

// Returns true if repr's synced properties have the same values as filter's
// same-named properties, using the filter's unchecked (pending Apply) values:
// those are what Apply commits, e.g. after a drag staged them.
bool RadVizPropertiesMatchFilter(pqDataRepresentation* repr, pqPipelineSource* filter)
{
  vtkSMProxy* filterProxy = filter->getProxy();
  vtkSMProxy* radVizProxy = repr->getProxy();
  for (const char* propertyName : ::SYNCED_PROPERTIES)
  {
    if (vtkSMUncheckedPropertyHelper(filterProxy, propertyName).GetDoubleArray() !=
      vtkSMPropertyHelper(radVizProxy, propertyName).GetDoubleArray())
    {
      return false;
    }
  }
  return true;
}

// Returns true if source is a MultiVariateToRGBA filter.
bool IsMultiVariateToRGBA(pqPipelineSource* source)
{
  vtkSMProxy* proxy = source ? source->getProxy() : nullptr;
  return proxy && strcmp(proxy->GetXMLGroup(), "filters") == 0 &&
    strcmp(proxy->GetXMLName(), "MultiVariateToRGBA") == 0;
}

// Returns true if source is a ResampleToImage filter.
bool IsResampleToImage(pqPipelineSource* source)
{
  vtkSMProxy* proxy = source ? source->getProxy() : nullptr;
  return proxy && strcmp(proxy->GetXMLGroup(), "filters") == 0 &&
    strcmp(proxy->GetXMLName(), "ResampleToImage") == 0;
}

// Returns the sources that take source's output as an input.
QList<pqPipelineSource*> Consumers(pqPipelineSource* source)
{
  QList<pqPipelineSource*> consumers;
  for (int port = 0; port < source->getNumberOfOutputPorts(); ++port)
  {
    for (pqPipelineSource* consumer : source->getOutputPort(port)->getConsumers())
    {
      if (!consumers.contains(consumer))
      {
        consumers.append(consumer);
      }
    }
  }
  return consumers;
}

// Returns the MultiVariateToRGBA filters that see the same data as a RadViz
// representation of source: for S -> [ResampleToImage ->] source, the filters
// in S -> [ResampleToImage ->] MultiVariateToRGBA.
QList<pqPipelineSource*> FindMultiVariateToRGBAFilters(pqPipelineSource* source)
{
  QList<pqPipelineSource*> filters;
  pqPipelineSource* base = source;
  if (::IsResampleToImage(source))
  {
    auto* resample = qobject_cast<pqPipelineFilter*>(source);
    pqOutputPort* input = resample ? resample->getAnyInput() : nullptr;
    base = input ? input->getSource() : nullptr;
  }
  if (!base)
  {
    return filters;
  }
  for (pqPipelineSource* consumer : ::Consumers(base))
  {
    QList<pqPipelineSource*> candidates{ consumer };
    if (::IsResampleToImage(consumer))
    {
      candidates = ::Consumers(consumer);
    }
    for (pqPipelineSource* candidate : candidates)
    {
      if (::IsMultiVariateToRGBA(candidate) && !filters.contains(candidate))
      {
        filters.append(candidate);
      }
    }
  }
  return filters;
}

// the filter name added to the view's name, saved with the view's state.
const char* const FILTER_ANNOTATION_KEY = "RadVizFilterAnnotation";

QString AnnotationSuffix(const QString& filterName)
{
  return QString(" (%1)").arg(filterName);
}
}

//-----------------------------------------------------------------------------
pqRadVizChartView::pqRadVizChartView(const QString& group, const QString& name,
  vtkSMContextViewProxy* viewModule, pqServer* server, QObject* p /*=nullptr*/)
  : Superclass(chartViewType(), group, name, viewModule, server, p)
  , FilterConnector(vtkSmartPointer<vtkEventQtSlotConnect>::New())
  , SourcePropertyConnector(vtkSmartPointer<vtkEventQtSlotConnect>::New())
  , InteractionSyncTimer(new QTimer(this))
{
  this->InteractionSyncTimer->setSingleShot(true);
  this->InteractionSyncTimer->setInterval(::INTERACTION_SYNC_DELAY_MS);
  QObject::connect(this->InteractionSyncTimer, &QTimer::timeout, this,
    &pqRadVizChartView::onInteractionSyncTimeout);

  QObject::connect(
    this, &pqView::representationAdded, this, &pqRadVizChartView::onRepresentationAdded);
  QObject::connect(
    this, &pqView::representationRemoved, this, &pqRadVizChartView::onRepresentationRemoved);
  QObject::connect(this, &pqView::representationVisibilityChanged, this,
    &pqRadVizChartView::onRepresentationVisibilityChanged);

  // state files may register the connected filter after this view: watch for
  // it so the connection can still be established.
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    &pqServerManagerModel::sourceAdded, this, &pqRadVizChartView::onSourceAdded);

  // tear down the property links before the connected filter is destroyed.
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    &pqServerManagerModel::sourceRemoved, this, &pqRadVizChartView::onSourceRemoved);
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    &pqServerManagerModel::nameChanged, this, &pqRadVizChartView::onNameChanged);

  // the view may already show a representation connected to a filter.
  for (int i = 0; i < this->getNumberOfRepresentations(); ++i)
  {
    this->onRepresentationAdded(this->getRepresentation(i));
  }
  this->updateConnection();
}

//-----------------------------------------------------------------------------
pqRadVizChartView::~pqRadVizChartView()
{
  // ~pqView detaches the representations, which emits representationVisibilityChanged.
  QObject::disconnect(this, &pqView::representationVisibilityChanged, this, nullptr);
}

//-----------------------------------------------------------------------------
QString pqRadVizChartView::filterAnnotation()
{
  const char* filterName = this->getProxy()->GetAnnotation(FILTER_ANNOTATION_KEY);
  // the user may have renamed the view since.
  if (filterName && this->getSMName().endsWith(::AnnotationSuffix(filterName)))
  {
    return QString::fromUtf8(filterName);
  }
  return QString();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::setFilterAnnotation(const QString& filterName)
{
  const QString name = this->getSMName();
  QString oldFilterName = this->filterAnnotation();
  // a name restored without the annotation, as from a Python state, may end with it.
  if (oldFilterName.isEmpty() && !filterName.isEmpty() &&
    name.endsWith(::AnnotationSuffix(filterName)))
  {
    oldFilterName = filterName;
  }
  QString newName =
    oldFilterName.isEmpty() ? name : name.chopped(::AnnotationSuffix(oldFilterName).size());
  if (filterName.isEmpty())
  {
    this->getProxy()->RemoveAnnotation(FILTER_ANNOTATION_KEY);
  }
  else
  {
    newName += ::AnnotationSuffix(filterName);
    this->getProxy()->SetAnnotation(FILTER_ANNOTATION_KEY, filterName.toUtf8().data());
  }
  if (newName != name)
  {
    this->rename(newName);
  }
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::showFilterComponents(
  vtkSMProxy* radVizProxy, vtkSMProxy* filterProxy, bool unchecked)
{
  std::string arrayName;
  vtkPVArrayInformation* arrayInfo = ::MultiVariateArrayInformation(filterProxy, arrayName);
  if (!radVizProxy || !arrayInfo)
  {
    return;
  }
  // the array's attributes for the filter's output; ResampleToImage turns
  // them into point data. Other inputs, as from a view connected by hand, keep them.
  vtkSMProxy* input = vtkSMPropertyHelper(radVizProxy, "Input").GetAsProxy();
  const bool resampled =
    input && input->GetXMLName() && std::strcmp(input->GetXMLName(), "ResampleToImage") == 0;
  const int fieldAssociation = resampled
    ? static_cast<int>(vtkDataObject::FIELD_ASSOCIATION_POINTS)
    : vtkSMPropertyHelper(filterProxy, ::MULTIVARIATE_ARRAY_PROPERTY).GetAsInt(3);
  vtkSMPropertyHelper(radVizProxy, ::FIELD_ASSOCIATION_PROPERTY).Set(fieldAssociation);
  const std::vector<int> visible =
    ::FilterVisibleComponents(filterProxy, arrayInfo->GetNumberOfComponents(), unchecked);
  std::vector<bool> shown(arrayInfo->GetNumberOfComponents(), false);
  vtkSMPropertyHelper seriesVisibility(radVizProxy, ::SERIES_VISIBILITY_PROPERTY);
  // elements: [name0, status0, name1, status1, ...], one pair per series.
  for (unsigned int i = 0; i + 1 < seriesVisibility.GetNumberOfElements(); i += 2)
  {
    const std::string name = seriesVisibility.GetAsString(i);
    const int c = ::SeriesComponent(name, arrayName, arrayInfo);
    const bool show = c >= 0 && std::binary_search(visible.begin(), visible.end(), c);
    seriesVisibility.SetStatus(name.c_str(), show ? 1 : 0);
    if (show)
    {
      shown[c] = true;
    }
  }
  // series not listed yet, as before the domain is updated.
  for (int c : visible)
  {
    if (!shown[c])
    {
      seriesVisibility.SetStatus(::ComponentSeriesName(arrayName, arrayInfo, c).c_str(), 1);
    }
  }
  radVizProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqRadVizChartView::isUpdatingSeries(vtkSMProxy* radVizProxy)
{
  auto* repr =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqDataRepresentation*>(
      radVizProxy);
  auto* view = repr ? qobject_cast<pqRadVizChartView*>(repr->getView()) : nullptr;
  return view && view->UpdatingSeriesVisibility;
}

//-----------------------------------------------------------------------------
bool pqRadVizChartView::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton)
    {
      this->Position = me->pos();
    }
  }
  else if (e->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if ((me->button() & Qt::RightButton) && this->Position)
    {
      QPoint newPos = me->pos();
      QPoint delta = newPos - *this->Position;
      QWidget* senderWidget = qobject_cast<QWidget*>(caller);
      if (delta.manhattanLength() < 3 && senderWidget != nullptr)
      {
        this->showContextMenu(senderWidget->mapToGlobal(newPos));
      }
      this->Position.reset();
    }
  }

  return this->Superclass::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::showContextMenu(const QPoint& globalPos)
{
  pqDataRepresentation* repr = this->radVizRepresentation();
  pqPipelineSource* connected = this->connectedFilter(repr);

  QMenu menu;
  menu.setObjectName("RadVizContextMenu");
  QAction* action;
  if (connected)
  {
    action = menu.addAction(tr("Disconnect from %1").arg(connected->getSMName()));
    action->setObjectName("DisconnectFromFilter");
    QObject::connect(action, &QAction::triggered, this, &pqRadVizChartView::disconnectFromFilter);
  }
  else
  {
    const QList<pqPipelineSource*> candidates = this->findConnectableFilters(repr);
    for (pqPipelineSource* candidate : candidates)
    {
      action = menu.addAction(tr("Connect to %1").arg(candidate->getSMName()));
      action->setObjectName(QString("ConnectTo%1").arg(candidate->getSMName()));
      QPointer<pqPipelineSource> filter = candidate;
      QObject::connect(action, &QAction::triggered, this,
        [this, filter]()
        {
          if (filter)
          {
            this->connectToFilter(filter);
          }
        });
    }
    if (candidates.isEmpty())
    {
      action = menu.addAction(tr("Connect to MultiVariateToRGBA"));
      action->setEnabled(false);
    }
  }
  menu.exec(globalPos);
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqRadVizChartView::radVizRepresentation() const
{
  for (int i = 0; i < this->getNumberOfRepresentations(); ++i)
  {
    auto* repr = qobject_cast<pqDataRepresentation*>(this->getRepresentation(i));
    if (repr && repr->isVisible())
    {
      return repr;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqRadVizChartView::connectedFilter(pqDataRepresentation* repr) const
{
  if (!repr)
  {
    return nullptr;
  }
  vtkSMProxy* radVizProxy = repr->getProxy();
  vtkSMProxy* filterProxy =
    vtkSMPropertyHelper(radVizProxy, pqRadVizChartView::RADVIZ_SOURCE_PROPERTY, /*quiet=*/true)
      .GetAsProxy();
  if (!filterProxy)
  {
    return nullptr;
  }
  return pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(
    filterProxy);
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> pqRadVizChartView::findConnectableFilters(pqDataRepresentation* repr) const
{
  return repr ? ::FindMultiVariateToRGBAFilters(repr->getInput()) : QList<pqPipelineSource*>();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::disconnectFromFilter()
{
  pqDataRepresentation* repr = this->radVizRepresentation();
  pqPipelineSource* filter = this->connectedFilter(repr);
  if (!repr || !filter)
  {
    return;
  }
  SCOPED_UNDO_SET(tr("Disconnect from %1").arg(filter->getSMName()));

  vtkSMProxy* radVizProxy = repr->getProxy();
  vtkSMPropertyHelper(radVizProxy, pqRadVizChartView::RADVIZ_SOURCE_PROPERTY).RemoveAllValues();
  radVizProxy->UpdateVTKObjects();
  this->updateConnection();
  Q_EMIT this->disconnectedFromFilter();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::disconnectAllFromFilter(pqPipelineSource* filter)
{
  if (!filter)
  {
    return;
  }
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  for (pqDataRepresentation* repr : smModel->findItems<pqDataRepresentation*>())
  {
    vtkSMProxy* radVizProxy = repr->getProxy();
    vtkSMPropertyHelper helper(radVizProxy, pqRadVizChartView::RADVIZ_SOURCE_PROPERTY, true);
    if (helper.GetAsProxy() != filter->getProxy())
    {
      continue;
    }
    helper.RemoveAllValues();
    radVizProxy->UpdateVTKObjects();
    if (auto* view = qobject_cast<pqRadVizChartView*>(repr->getView()))
    {
      const bool wasLinked = view->LinkedFilter == filter;
      view->updateConnection();
      if (wasLinked)
      {
        Q_EMIT view->disconnectedFromFilter();
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::connectToFilter(pqPipelineSource* filter)
{
  pqDataRepresentation* repr = this->radVizRepresentation();
  if (!repr || !filter)
  {
    return;
  }
  SCOPED_UNDO_SET(tr("Connect to %1").arg(filter->getSMName()));

  vtkSMProxy* radVizProxy = repr->getProxy();
  vtkSMPropertyHelper(radVizProxy, pqRadVizChartView::RADVIZ_SOURCE_PROPERTY)
    .Set(filter->getProxy());
  radVizProxy->UpdateVTKObjects();
  // the view takes the filter's values and components, as with Show RadViz.
  this->updateConnection();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onRepresentationAdded(pqRepresentation* repr)
{
  if (vtkSMProperty* sourceProperty =
        repr ? repr->getProxy()->GetProperty(pqRadVizChartView::RADVIZ_SOURCE_PROPERTY) : nullptr)
  {
    this->SourcePropertyConnector->Connect(
      sourceProperty, vtkCommand::ModifiedEvent, this, SLOT(onSourcePropertyModified()));
  }
  this->updateConnection();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onRepresentationVisibilityChanged(pqRepresentation* repr, bool)
{
  // a representation being removed is detached first: onRepresentationRemoved() handles it.
  if (repr->getView() == this)
  {
    this->updateConnection();
  }
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onRepresentationRemoved(pqRepresentation* repr)
{
  if (vtkSMProperty* sourceProperty =
        repr ? repr->getProxy()->GetProperty(pqRadVizChartView::RADVIZ_SOURCE_PROPERTY) : nullptr)
  {
    this->SourcePropertyConnector->Disconnect(
      sourceProperty, vtkCommand::ModifiedEvent, this, SLOT(onSourcePropertyModified()));
  }
  if (repr != this->LinkedRepresentation)
  {
    return;
  }
  const QString filterName = this->LinkedFilter ? this->LinkedFilter->getSMName() : QString();
  this->teardownLink();
  // drop the annotation later, as onSourceRemoved() does, unless the view was relinked.
  QTimer::singleShot(0, this,
    [this, filterName]()
    {
      if (!this->LinkedFilter && !filterName.isEmpty() && this->filterAnnotation() == filterName)
      {
        this->setFilterAnnotation(QString());
      }
    });
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onSourceAdded(pqPipelineSource*)
{
  this->updateConnection();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onSourceRemoved(pqPipelineSource* source)
{
  if (source == this->LinkedFilter)
  {
    this->teardownLink();
  }
  // drop the removed filter's annotation, even if the link is already gone:
  // deleting the filter removes the representation first. Later, as renaming
  // now would break a session reset, which destroys this view. A view linked
  // to another filter, maybe with the same name, keeps that filter's annotation.
  const QString sourceName = source->getSMName();
  QTimer::singleShot(0, this,
    [this, sourceName]()
    {
      if (!this->LinkedFilter && this->filterAnnotation() == sourceName)
      {
        this->setFilterAnnotation(QString());
      }
    });
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onNameChanged(pqServerManagerModelItem* item)
{
  if (this->LinkedFilter && item == this->LinkedFilter)
  {
    this->setFilterAnnotation(this->LinkedFilter->getSMName());
  }
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onSourcePropertyModified()
{
  QTimer::singleShot(0, this, &pqRadVizChartView::updateConnection);
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::updateConnection()
{
  pqDataRepresentation* repr = this->radVizRepresentation();
  pqPipelineSource* filter = this->connectedFilter(repr);
  if (repr == this->LinkedRepresentation && filter == this->LinkedFilter)
  {
    return;
  }

  const QString oldFilterName = this->LinkedFilter ? this->LinkedFilter->getSMName() : QString();
  this->teardownLink();
  if (repr && filter)
  {
    this->establishLink(repr, filter);
  }

  // the view's name shows the filter its visible representation is connected to.
  if (this->LinkedFilter)
  {
    this->setFilterAnnotation(this->LinkedFilter->getSMName());
  }
  else if (!oldFilterName.isEmpty() && this->filterAnnotation() == oldFilterName)
  {
    this->setFilterAnnotation(QString());
  }
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::establishLink(pqDataRepresentation* repr, pqPipelineSource* filter)
{
  vtkSMProxy* filterProxy = filter->getProxy();
  vtkSMProxy* radVizProxy = repr->getProxy();
  // the view starts with the filter's pending values, so staged edits are kept.
  for (const char* propertyName : ::SYNCED_PROPERTIES)
  {
    vtkSMPropertyHelper filterHelper(filterProxy, propertyName);
    filterHelper.SetUseUnchecked(true);
    vtkSMPropertyHelper(radVizProxy, propertyName).Copy(filterHelper);
  }
  radVizProxy->UpdateVTKObjects();
  for (const char* propertyName : ::SYNCED_PROPERTIES)
  {
    // filter -> RadViz representation: live-updates the view.
    this->FilterConnector->Connect(filterProxy->GetProperty(propertyName),
      vtkCommand::ModifiedEvent, this, SLOT(syncRadVizFromFilter(vtkObject*)));

    // RadViz representation -> filter: Display panel, Python or state reload.
    this->FilterConnector->Connect(radVizProxy->GetProperty(propertyName),
      vtkCommand::ModifiedEvent, this, SLOT(syncFilterFromRadViz()));
  }

  // visible components: series -> filter.
  this->FilterConnector->Connect(radVizProxy->GetProperty(::SERIES_VISIBILITY_PROPERTY),
    vtkCommand::ModifiedEvent, this, SLOT(syncFilterFromSeries()));
  // filter -> representation: the filter's pending synced properties and visible
  // components. Setting a value (Apply, Python, state) also sets the pending one.
  this->FilterConnector->Connect(filterProxy->GetProperty(::VISIBLE_COMPONENTS_PROPERTY),
    vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(previewFilter()));
  for (const char* propertyName : ::SYNCED_PROPERTIES)
  {
    this->FilterConnector->Connect(filterProxy->GetProperty(propertyName),
      vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(previewFilter()));
  }

  // before the first Apply, the filter's output has no components to show.
  this->FilterConnector->Connect(
    filterProxy, vtkCommand::UpdateDataEvent, this, SLOT(onFilterDataUpdated()));

  // Track interactive drags of the vertices or lens
  this->FilterConnector->Connect(
    radVizProxy, vtkCommand::StartInteractionEvent, this, SLOT(onRadVizStartInteraction()));
  this->FilterConnector->Connect(
    radVizProxy, vtkCommand::InteractionEvent, this, SLOT(onRadVizInteraction()));
  this->FilterConnector->Connect(
    radVizProxy, vtkCommand::EndInteractionEvent, this, SLOT(onRadVizEndInteraction()));

  this->LinkedRepresentation = repr;
  this->LinkedFilter = filter;
  this->syncSeriesFromFilter(true);
  this->render();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::teardownLink()
{
  this->FilterConnector->Disconnect();
  this->InteractionSyncTimer->stop();
  this->InteractionInProgress = false;
  this->LinkedRepresentation = nullptr;
  this->LinkedFilter = nullptr;
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onRadVizStartInteraction()
{
  this->InteractionInProgress = true;
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onRadVizInteraction()
{
  this->InteractionSyncTimer->start();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onInteractionSyncTimeout()
{
  if (!this->LinkedRepresentation || !this->LinkedFilter)
  {
    return;
  }
  // mid-drag, the proxy's properties still hold the pre-drag values: update
  // them. syncFilterFromRadViz() ignores the resulting ModifiedEvents.
  if (auto* radVizProxy =
        vtkSMRadVizRepresentationProxy::SafeDownCast(this->LinkedRepresentation->getProxy()))
  {
    radVizProxy->UpdatePropertiesFromRepresentation();
  }
  this->stageFilterFromRadViz();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onRadVizEndInteraction()
{
  this->InteractionInProgress = false;
  // the proxy already has the drag's final result. An idle timer means it was
  // already staged, or nothing changed.
  if (this->InteractionSyncTimer->isActive())
  {
    this->stageFilterFromRadViz();
  }
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::syncRadVizFromFilter(vtkObject* filterProperty)
{
  if (!this->LinkedRepresentation)
  {
    return;
  }
  auto* property = vtkSMProperty::SafeDownCast(filterProperty);
  vtkSMProxy* radVizProxy = this->LinkedRepresentation->getProxy();
  vtkSMProperty* radVizProperty =
    property ? radVizProxy->GetProperty(property->GetXMLName()) : nullptr;
  if (!radVizProperty)
  {
    return;
  }
  // an Apply sets the filter's properties one element at a time: echoing the
  // partly applied values back would stage them and highlight Apply again.
  this->withReverseSyncSuspended(
    [&]()
    {
      radVizProperty->Copy(property);
      radVizProxy->UpdateVTKObjects();
    });
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::syncFilterFromRadViz()
{
  if (!this->LinkedRepresentation || !this->LinkedFilter || this->ReverseSyncSuspendCount > 0 ||
    this->InteractionInProgress ||
    ::RadVizPropertiesMatchFilter(this->LinkedRepresentation, this->LinkedFilter))
  {
    return;
  }
  this->stageFilterFromRadViz();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::stageFilterFromRadViz()
{
  this->InteractionSyncTimer->stop();
  if (!this->LinkedRepresentation || !this->LinkedFilter)
  {
    return;
  }
  this->StagingFilter = true;
  ::CopyRadVizPropertiesToFilter(this->LinkedRepresentation, this->LinkedFilter, true);
  this->StagingFilter = false;
  Q_EMIT this->filterPropertiesStagedFromView();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::syncSeriesFromFilter(bool unchecked)
{
  if (!this->LinkedRepresentation || !this->LinkedFilter)
  {
    return;
  }
  this->UpdatingSeriesVisibility = true;
  pqRadVizChartView::showFilterComponents(
    this->LinkedRepresentation->getProxy(), this->LinkedFilter->getProxy(), unchecked);
  this->UpdatingSeriesVisibility = false;
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::onFilterDataUpdated()
{
  this->syncSeriesFromFilter(true);
  // the update may come from this view's own render, before the series changed.
  this->render();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::previewFilter()
{
  // the representation already has the values being staged
  if (!this->LinkedRepresentation || !this->LinkedFilter || this->StagingFilter)
  {
    return;
  }
  vtkSMProxy* radVizProxy = this->LinkedRepresentation->getProxy();
  vtkSMProxy* filterProxy = this->LinkedFilter->getProxy();
  this->UpdatingSeriesVisibility = true;
  this->withReverseSyncSuspended(
    [&]()
    {
      for (const char* propertyName : ::SYNCED_PROPERTIES)
      {
        vtkSMPropertyHelper filterHelper(filterProxy, propertyName);
        filterHelper.SetUseUnchecked(true);
        vtkSMPropertyHelper(radVizProxy, propertyName).Copy(filterHelper);
      }
      // showFilterComponents() returns early if the array is not in the filter's output yet
      radVizProxy->UpdateVTKObjects();
      pqRadVizChartView::showFilterComponents(radVizProxy, filterProxy, true);
    });
  this->UpdatingSeriesVisibility = false;
  this->render();
}

//-----------------------------------------------------------------------------
void pqRadVizChartView::syncFilterFromSeries()
{
  if (!this->LinkedRepresentation || !this->LinkedFilter || this->UpdatingSeriesVisibility)
  {
    return;
  }
  vtkSMProxy* radVizProxy = this->LinkedRepresentation->getProxy();
  vtkSMProxy* filterProxy = this->LinkedFilter->getProxy();
  std::string arrayName;
  vtkPVArrayInformation* arrayInfo = ::MultiVariateArrayInformation(filterProxy, arrayName);
  if (!arrayInfo)
  {
    return;
  }
  const int numComponents = arrayInfo->GetNumberOfComponents();
  std::vector<int> visible;
  std::vector<std::string> others;
  vtkSMPropertyHelper seriesVisibility(radVizProxy, ::SERIES_VISIBILITY_PROPERTY);
  for (unsigned int i = 0; i + 1 < seriesVisibility.GetNumberOfElements(); i += 2)
  {
    if (seriesVisibility.GetAsInt(i + 1) == 0)
    {
      continue;
    }
    const std::string name = seriesVisibility.GetAsString(i);
    const int component = ::SeriesComponent(name, arrayName, arrayInfo);
    if (component < 0)
    {
      others.push_back(name);
    }
    else
    {
      visible.push_back(component);
    }
  }
  // a component is visible if any of its blocks is.
  std::sort(visible.begin(), visible.end());
  visible.erase(std::unique(visible.begin(), visible.end()), visible.end());
  if (visible.size() < ::MIN_VISIBLE_COMPONENTS)
  {
    // too few components left: keep the filter's components, as previewed.
    this->syncSeriesFromFilter(true);
    return;
  }
  if (!others.empty())
  {
    // the filter only processes components of its MultiVariateArray.
    this->UpdatingSeriesVisibility = true;
    for (const std::string& name : others)
    {
      seriesVisibility.SetStatus(name.c_str(), 0);
    }
    radVizProxy->UpdateVTKObjects();
    this->UpdatingSeriesVisibility = false;
  }
  if (visible == ::FilterVisibleComponents(filterProxy, numComponents, true))
  {
    return;
  }

  vtkSMPropertyHelper visibleHelper(filterProxy, ::VISIBLE_COMPONENTS_PROPERTY);
  visibleHelper.SetUseUnchecked(true);
  if (static_cast<int>(visible.size()) == numComponents)
  {
    visibleHelper.SetNumberOfElements(0);
  }
  else
  {
    visibleHelper.Set(visible.data(), static_cast<unsigned int>(visible.size()));
  }
  // the partition refers to the visible components: clear it. Each property fires
  // ModifiedEvent: stage once, after all of them are cleared.
  this->withReverseSyncSuspended(
    [radVizProxy]()
    {
      for (const char* propertyName :
        { ::PARTITION_INDICES_PROPERTY, ::PARTITION_OFFSETS_PROPERTY, ::CUSTOM_RANGE_PROPERTY })
      {
        vtkSMPropertyHelper(radVizProxy, propertyName).SetNumberOfElements(0);
      }
      radVizProxy->UpdateVTKObjects();
    });
  this->stageFilterFromRadViz();
}
