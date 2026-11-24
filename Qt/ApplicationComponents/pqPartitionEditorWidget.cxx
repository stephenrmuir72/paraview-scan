// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPartitionEditorWidget.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqRadVizChartView.h"
#include "pqRescaleScalarRangeToCustomDialog.h"
#include "pqUndoStack.h"
#include "pqWidgetUtilities.h"
#include "vtkChartRepresentation.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMDocumentation.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QListWidget>
#include <QMap>
#include <QPalette>
#include <QSet>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStringList>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>
#include <vector>

namespace
{
// Returns true if property has a pending (unchecked) value that differs from
// its applied one, as after an edit in the Properties panel. Setting a value from
// Python or a state file sets both.
bool HasPendingValue(vtkSMProperty* property)
{
  if (!property)
  {
    return false;
  }
  vtkSMPropertyHelper applied(property, /*quiet=*/true);
  vtkSMUncheckedPropertyHelper pending(property, /*quiet=*/true);
  const unsigned int numElems = applied.GetNumberOfElements();
  if (numElems != pending.GetNumberOfElements())
  {
    return true;
  }
  for (unsigned int i = 0; i < numElems; ++i)
  {
    if (applied.GetAsVariant(i) != pending.GetAsVariant(i))
    {
      return true;
    }
  }
  return false;
}

// Marks a QListWidgetItem as a separator, rather than holding a component
// index, in its Qt::UserRole data.
const int SEPARATOR_VALUE = -1;

// The list grows to show at most this many rows (components and
// separators), and scrolls past that.
const int MAX_VISIBLE_ROWS = 12;

bool isSeparator(QListWidgetItem* item)
{
  return item && item->data(Qt::UserRole).toInt() == ::SEPARATOR_VALUE;
}

// Marks 'item' (already inserted into 'listWidget') as a separator and
// gives it a widget rendering as a horizontal line spanning the full width
// of the list, rather than a text label.
void styleSeparatorItem(QListWidget* listWidget, QListWidgetItem* item)
{
  item->setData(Qt::UserRole, ::SEPARATOR_VALUE);
  item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);

  // thicken the line and add vertical margins so it reads as a group boundary.
  auto* line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Plain);
  line->setLineWidth(2);
  line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  QPalette palette = line->palette();
  palette.setColor(QPalette::WindowText, palette.color(QPalette::Text));
  line->setPalette(palette);

  auto* container = new QWidget();
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(4, 6, 4, 6);
  layout->addWidget(line);

  item->setSizeHint(container->sizeHint());
  listWidget->setItemWidget(item, container);
}

// Returns true if 'indices' is a permutation of [0, numComponents) and
// 'offsets' is strictly increasing, positive, and ends at numComponents,
// i.e. together they describe a valid partition of numComponents
// components (see vtkMultiVariateToRGBA::SetPartitionIndices).
bool isValidPartition(
  const QList<QVariant>& indices, const QList<QVariant>& offsets, int numComponents)
{
  if (numComponents <= 0 || indices.size() != numComponents || offsets.isEmpty())
  {
    return false;
  }
  std::vector<int> sorted;
  sorted.reserve(static_cast<size_t>(indices.size()));
  for (const QVariant& v : indices)
  {
    sorted.push_back(v.toInt());
  }
  std::sort(sorted.begin(), sorted.end());
  for (int i = 0; i < numComponents; ++i)
  {
    if (sorted[static_cast<size_t>(i)] != i)
    {
      return false;
    }
  }
  int previous = 0;
  for (const QVariant& v : offsets)
  {
    int offset = v.toInt();
    if (offset <= previous)
    {
      return false;
    }
    previous = offset;
  }
  return previous == numComponents;
}

// Returns the data information of the (unchecked) first input in
// 'inputProperty', or nullptr if not available.
vtkPVDataInformation* inputDataInformation(vtkSMProperty* inputProperty)
{
  if (!inputProperty)
  {
    return nullptr;
  }
  vtkSMUncheckedPropertyHelper inputHelper(inputProperty);
  vtkSMSourceProxy* source = inputHelper.GetNumberOfElements() > 0
    ? vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy(0))
    : nullptr;
  return source ? source->GetDataInformation(inputHelper.GetOutputPort()) : nullptr;
}

// For a (name, visible) pairs StringVectorProperty (e.g. SeriesVisibility),
// returns the names, in order, of the entries currently marked visible.
QStringList visibleNames(vtkSMStringVectorProperty* svp)
{
  QStringList names;
  unsigned int numElems = svp->GetNumberOfElements();
  for (unsigned int i = 0; i + 1 < numElems; i += 2)
  {
    bool visible = QString(svp->GetElement(i + 1)).toInt() != 0;
    if (visible)
    {
      names.append(QString::fromUtf8(svp->GetElement(i)));
    }
  }
  return names;
}

// For a (name, visible) pairs StringVectorProperty, returns all the names.
QSet<QString> allNames(vtkSMStringVectorProperty* svp)
{
  QSet<QString> names;
  unsigned int numElems = svp->GetNumberOfElements();
  for (unsigned int i = 0; i + 1 < numElems; i += 2)
  {
    names.insert(QString::fromUtf8(svp->GetElement(i)));
  }
  return names;
}
}

//-----------------------------------------------------------------------------
pqPartitionEditorWidget::pqPartitionEditorWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , VTKConnector(vtkEventQtSlotConnect::New())
{
  this->ListWidget = new QListWidget(this);
  this->ListWidget->setObjectName("PartitionList");
  this->ListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  if (smgroup->GetDocumentation())
  {
    QString doc = QCoreApplication::translate(
      "ServerManagerXML", smgroup->GetDocumentation()->GetDescription());
    this->ListWidget->setToolTip(pqWidgetUtilities::formatTooltip(doc));
  }

  this->UpButton = new QToolButton(this);
  this->UpButton->setObjectName("MoveUp");
  this->UpButton->setIcon(this->style()->standardIcon(QStyle::SP_ArrowUp));
  this->UpButton->setToolTip(tr("Move the selected component up, possibly across a separator"));

  this->DownButton = new QToolButton(this);
  this->DownButton->setObjectName("MoveDown");
  this->DownButton->setIcon(this->style()->standardIcon(QStyle::SP_ArrowDown));
  this->DownButton->setToolTip(tr("Move the selected component down, possibly across a separator"));

  this->AddSeparatorButton = new QToolButton(this);
  this->AddSeparatorButton->setObjectName("AddSeparator");
  this->AddSeparatorButton->setText(tr("Add Separator"));
  this->AddSeparatorButton->setToolTip(
    tr("Insert a new partition separator right after the selected component"));

  this->RemoveSeparatorButton = new QToolButton(this);
  this->RemoveSeparatorButton->setObjectName("RemoveSeparator");
  this->RemoveSeparatorButton->setText(tr("Remove Separator"));
  this->RemoveSeparatorButton->setToolTip(tr("Remove the selected partition separator"));

  this->RescaleButton = new QToolButton(this);
  this->RescaleButton->setObjectName("Rescale");
  this->RescaleButton->setIcon(QIcon(":/pqWidgets/Icons/pqResetRange.svg"));
  this->RescaleButton->setToolTip(tr("Rescale the group of the selected element to data range"));

  this->RescaleCustomButton = new QToolButton(this);
  this->RescaleCustomButton->setObjectName("RescaleCustom");
  this->RescaleCustomButton->setIcon(QIcon(":/pqWidgets/Icons/pqResetRangeCustom.svg"));
  this->RescaleCustomButton->setToolTip(
    tr("Rescale the group of the selected element to custom range"));

  QHBoxLayout* rescaleLayout = new QHBoxLayout();
  rescaleLayout->addWidget(this->RescaleButton);
  rescaleLayout->addWidget(this->RescaleCustomButton);
  rescaleLayout->addStretch();

  QVBoxLayout* buttonsLayout = new QVBoxLayout();
  buttonsLayout->addWidget(this->UpButton);
  buttonsLayout->addWidget(this->DownButton);
  buttonsLayout->addSpacing(12);
  buttonsLayout->addWidget(this->AddSeparatorButton);
  buttonsLayout->addWidget(this->RemoveSeparatorButton);
  buttonsLayout->addSpacing(12);
  buttonsLayout->addLayout(rescaleLayout);
  buttonsLayout->addStretch();

  QHBoxLayout* mainLayout = new QHBoxLayout(this);
  mainLayout->addWidget(this->ListWidget);
  mainLayout->addLayout(buttonsLayout);
  this->ButtonsHeight = buttonsLayout->sizeHint().height();

  this->connect(this->UpButton, &QToolButton::clicked, this, &pqPartitionEditorWidget::moveUp);
  this->connect(this->DownButton, &QToolButton::clicked, this, &pqPartitionEditorWidget::moveDown);
  this->connect(
    this->AddSeparatorButton, &QToolButton::clicked, this, &pqPartitionEditorWidget::addSeparator);
  this->connect(this->RemoveSeparatorButton, &QToolButton::clicked, this,
    &pqPartitionEditorWidget::removeSeparator);
  this->connect(
    this->RescaleButton, &QToolButton::clicked, this, &pqPartitionEditorWidget::rescaleToDataRange);
  this->connect(this->RescaleCustomButton, &QToolButton::clicked, this,
    &pqPartitionEditorWidget::rescaleToCustomRange);
  this->connect(this->ListWidget, &QListWidget::itemSelectionChanged, this,
    &pqPartitionEditorWidget::updateButtonStates);

  // two-way link between Qt properties and SM properties
  vtkSMProperty* indicesProperty = smgroup->GetProperty("Indices");
  vtkSMProperty* offsetsProperty = smgroup->GetProperty("Offsets");
  this->addPropertyLink(
    this, "partitionIndices", SIGNAL(partitionIndicesChanged()), indicesProperty);
  this->addPropertyLink(
    this, "partitionOffsets", SIGNAL(partitionOffsetsChanged()), offsetsProperty);
  if (vtkSMProperty* customRangeProperty = smgroup->GetProperty("CustomRange"))
  {
    this->addPropertyLink(this, "customRange", SIGNAL(customRangeChanged()), customRangeProperty);
  }
  else
  {
    this->RescaleButton->hide();
    this->RescaleCustomButton->hide();
  }

  // a NumberOfComponentsDomain or a (name, visible) StringVectorProperty
  this->ComponentsProperty = smgroup->GetProperty("Components");
  if (this->ComponentsProperty)
  {
    // N changes through DomainModifiedEvent (NumberOfComponentsDomain) or through
    // the value of a (name, visible) property such as SeriesVisibility. Setting a
    // value also fires UncheckedPropertyModifiedEvent.
    this->VTKConnector->Connect(
      this->ComponentsProperty, vtkCommand::DomainModifiedEvent, this, SLOT(componentsChanged()));
    this->VTKConnector->Connect(this->ComponentsProperty,
      vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(componentsChanged()));
  }

  this->VisibleComponentsProperty = smgroup->GetProperty("VisibleComponents");
  if (this->VisibleComponentsProperty)
  {
    this->VTKConnector->Connect(this->VisibleComponentsProperty,
      vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(componentsChanged()));
  }

  this->ComponentsKey = this->componentsKey();
  this->seriesEdited();
  this->renderList();
}

//-----------------------------------------------------------------------------
pqPartitionEditorWidget::~pqPartitionEditorWidget()
{
  this->VTKConnector->Delete();
}

//-----------------------------------------------------------------------------
int pqPartitionEditorWidget::numberOfComponents() const
{
  if (!this->ComponentsProperty)
  {
    return 0;
  }
  if (auto* domain = this->ComponentsProperty->FindDomain<vtkSMEnumerationDomain>())
  {
    return static_cast<int>(
      this->visibleComponentIndices(static_cast<int>(domain->GetNumberOfEntries())).size());
  }
  if (auto* svp = vtkSMStringVectorProperty::SafeDownCast(this->ComponentsProperty))
  {
    return this->naturalOrderVisibleNames(svp).size();
  }
  return 0;
}

//-----------------------------------------------------------------------------
std::vector<int> pqPartitionEditorWidget::visibleComponentIndices(int total) const
{
  std::vector<int> indices;
  if (this->VisibleComponentsProperty)
  {
    vtkSMUncheckedPropertyHelper helper(this->VisibleComponentsProperty);
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
  }
  if (indices.empty())
  {
    for (int i = 0; i < total; ++i)
    {
      indices.push_back(i);
    }
  }
  return indices;
}

//-----------------------------------------------------------------------------
QString pqPartitionEditorWidget::componentLabel(int value) const
{
  if (this->ComponentsProperty)
  {
    if (auto* domain = this->ComponentsProperty->FindDomain<vtkSMEnumerationDomain>())
    {
      // value is the index among the visible components
      const std::vector<int> visible =
        this->visibleComponentIndices(static_cast<int>(domain->GetNumberOfEntries()));
      const int component =
        value >= 0 && value < static_cast<int>(visible.size()) ? visible[value] : value;
      if (const char* name = domain->GetEntryTextForValue(component))
      {
        if (name[0] != 0)
        {
          return tr("%1: %2").arg(value).arg(QString::fromUtf8(name));
        }
      }
    }
    else if (auto* svp = vtkSMStringVectorProperty::SafeDownCast(this->ComponentsProperty))
    {
      QStringList names = this->naturalOrderVisibleNames(svp);
      if (value >= 0 && value < names.size())
      {
        return tr("%1: %2").arg(value).arg(names[value]);
      }
    }
  }
  return QString::number(value);
}

//-----------------------------------------------------------------------------
QStringList pqPartitionEditorWidget::naturalOrderVisibleNames(
  vtkSMStringVectorProperty* svp, std::vector<std::array<double, 2>>* ranges) const
{
  QStringList fallback = ::visibleNames(svp);
  const std::array<double, 2> unknownRange = { 1, -1 };
  if (ranges)
  {
    ranges->assign(fallback.size(), unknownRange);
  }

  vtkSMProxy* smproxy = this->proxy();
  vtkSMProperty* inputProperty = smproxy ? smproxy->GetProperty("Input") : nullptr;
  vtkSMProperty* fieldAssocProperty = smproxy ? smproxy->GetProperty("FieldAssociation") : nullptr;
  if (!inputProperty || !fieldAssocProperty)
  {
    return fallback;
  }

  vtkPVDataInformation* dataInfo = ::inputDataInformation(inputProperty);
  if (!dataInfo)
  {
    return fallback;
  }
  // a composite input keeps the SeriesVisibility order rather than guess a
  // natural order: only the ranges, over all its blocks, are looked up.
  const bool composite = dataInfo->IsCompositeDataSet();

  int fieldAssociation = vtkSMUncheckedPropertyHelper(fieldAssocProperty).GetAsInt();
  vtkPVDataSetAttributesInformation* dsa = dataInfo->GetAttributeInformation(fieldAssociation);
  if (!dsa)
  {
    return fallback;
  }

  auto* chartRepr = vtkChartRepresentation::SafeDownCast(smproxy->GetClientSideObject());
  QSet<QString> visible(fallback.begin(), fallback.end());
  QStringList result;
  std::vector<std::array<double, 2>> resultRanges;
  QMap<QString, std::array<double, 2>> compositeRanges;
  auto appendIfVisible = [&](const std::string& name, vtkPVArrayInformation* arrayInfo, int comp)
  {
    QString qname = QString::fromStdString(name);
    if (visible.remove(qname))
    {
      std::array<double, 2> range;
      arrayInfo->GetComponentFiniteRange(comp, range.data());
      if (composite)
      {
        compositeRanges[qname] = range;
        return;
      }
      result.append(qname);
      resultRanges.push_back(range);
    }
  };
  for (int i = 0; i < dsa->GetNumberOfArrays(); ++i)
  {
    vtkPVArrayInformation* arrayInfo = dsa->GetArrayInformation(i);
    const char* name = arrayInfo ? arrayInfo->GetName() : nullptr;
    if (!name)
    {
      continue;
    }
    if (arrayInfo->GetNumberOfComponents() > 1 && chartRepr)
    {
      // series names of a split multi-component array, in component order, then
      // its magnitude (CreateMangledName's component N, range component -1).
      const int numComponents = arrayInfo->GetNumberOfComponents();
      for (int c = 0; c <= numComponents; ++c)
      {
        appendIfVisible(chartRepr->GetDefaultSeriesLabel(
                          std::string(), vtkSMArrayListDomain::CreateMangledName(arrayInfo, c)),
          arrayInfo, c < numComponents ? c : -1);
      }
    }
    else
    {
      appendIfVisible(name, arrayInfo, 0);
    }
  }
  // names not found are appended in SeriesVisibility order rather than dropped.
  for (const QString& leftover : fallback)
  {
    if (composite)
    {
      result.append(leftover);
      resultRanges.push_back(compositeRanges.value(leftover, unknownRange));
    }
    else if (visible.contains(leftover))
    {
      result.append(leftover);
      resultRanges.push_back(unknownRange);
    }
  }
  if (ranges)
  {
    *ranges = resultRanges;
  }
  return result;
}

//-----------------------------------------------------------------------------
std::vector<std::array<double, 2>> pqPartitionEditorWidget::componentRanges() const
{
  std::vector<std::array<double, 2>> ranges;
  if (!this->ComponentsProperty)
  {
    return ranges;
  }
  if (auto* domain = this->ComponentsProperty->FindDomain<vtkSMEnumerationDomain>())
  {
    // the components of the array selected by the domain's ArraySelection
    vtkPVDataInformation* dataInfo = ::inputDataInformation(domain->GetRequiredProperty("Input"));
    vtkSMProperty* arrayProperty = domain->GetRequiredProperty("ArraySelection");
    if (!dataInfo || !arrayProperty)
    {
      return ranges;
    }
    vtkSMUncheckedPropertyHelper arrayHelper(arrayProperty);
    const unsigned int numElems = arrayHelper.GetNumberOfElements();
    if (numElems == 0)
    {
      return ranges;
    }
    // the array name is last: [idx, port, connection, association, name]
    const char* arrayName = arrayHelper.GetAsString(numElems - 1);
    const int association = numElems == 5 ? arrayHelper.GetAsInt(3) : vtkDataObject::POINT;
    vtkPVArrayInformation* arrayInfo = dataInfo->GetArrayInformation(arrayName, association);
    if (!arrayInfo)
    {
      return ranges;
    }
    for (int c : this->visibleComponentIndices(arrayInfo->GetNumberOfComponents()))
    {
      std::array<double, 2> range;
      arrayInfo->GetComponentFiniteRange(c, range.data());
      ranges.push_back(range);
    }
  }
  else if (auto* svp = vtkSMStringVectorProperty::SafeDownCast(this->ComponentsProperty))
  {
    this->naturalOrderVisibleNames(svp, &ranges);
  }
  return ranges;
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqPartitionEditorWidget::arrayProperty() const
{
  auto* domain = this->ComponentsProperty
    ? this->ComponentsProperty->FindDomain<vtkSMEnumerationDomain>()
    : nullptr;
  return domain ? domain->GetRequiredProperty("ArraySelection") : nullptr;
}

//-----------------------------------------------------------------------------
QStringList pqPartitionEditorWidget::componentsKey() const
{
  QStringList key;
  const int numComponents = this->numberOfComponents();
  if (numComponents == 0)
  {
    return key;
  }
  if (vtkSMProperty* arrayProperty = this->arrayProperty())
  {
    vtkSMUncheckedPropertyHelper arrayHelper(arrayProperty);
    const unsigned int numElems = arrayHelper.GetNumberOfElements();
    if (numElems > 0)
    {
      // the array name is last: [idx, port, connection, association, name]
      key.append(QString::fromUtf8(arrayHelper.GetAsString(numElems - 1)));
    }
  }
  for (int i = 0; i < numComponents; ++i)
  {
    key.append(this->componentLabel(i));
  }
  return key;
}

//-----------------------------------------------------------------------------
int pqPartitionEditorWidget::selectedGroup() const
{
  QList<QListWidgetItem*> selected = this->ListWidget->selectedItems();
  if (selected.isEmpty())
  {
    return -1;
  }
  // a separator belongs to the group it closes
  const int row = this->ListWidget->row(selected[0]);
  int group = 0;
  for (int r = 0; r < row; ++r)
  {
    if (::isSeparator(this->ListWidget->item(r)))
    {
      ++group;
    }
  }
  return group;
}

//-----------------------------------------------------------------------------
bool pqPartitionEditorWidget::groupDataRange(int group, double range[2]) const
{
  const std::vector<std::array<double, 2>> ranges = this->componentRanges();
  range[0] = std::numeric_limits<double>::max();
  range[1] = std::numeric_limits<double>::lowest();
  int currentGroup = 0;
  for (int row = 0; row < this->ListWidget->count() && currentGroup <= group; ++row)
  {
    QListWidgetItem* item = this->ListWidget->item(row);
    if (::isSeparator(item))
    {
      ++currentGroup;
    }
    else if (currentGroup == group)
    {
      const int value = item->data(Qt::UserRole).toInt();
      if (value < 0 || value >= static_cast<int>(ranges.size()) ||
        ranges[value][0] > ranges[value][1])
      {
        return false;
      }
      range[0] = std::min(range[0], ranges[value][0]);
      range[1] = std::max(range[1], ranges[value][1]);
    }
  }
  return range[0] <= range[1];
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::setGroupCustomRange(int group, const double range[2])
{
  const int numGroups = this->separatorCount();
  if (group < 0 || group >= numGroups)
  {
    qCritical().noquote() << tr("Invalid partition group %1.").arg(group);
    return;
  }
  QList<QVariant> customRange = this->CachedCustomRange;
  if (customRange.size() != 2 * numGroups)
  {
    customRange.clear();
    for (int g = 0; g < numGroups; ++g)
    {
      double dataRange[2];
      if (!this->groupDataRange(g, dataRange))
      {
        qCritical().noquote() << tr("Cannot compute the data range of partition group %1.").arg(g);
        return;
      }
      customRange.append(dataRange[0]);
      customRange.append(dataRange[1]);
    }
  }
  customRange[2 * group] = range[0];
  customRange[2 * group + 1] = range[1];
  if (customRange != this->CachedCustomRange)
  {
    this->CachedCustomRange = customRange;
    this->updateButtonStates();
    Q_EMIT this->customRangeChanged();
  }
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::rescaleToDataRange()
{
  const int group = this->selectedGroup();
  if (group < 0)
  {
    return;
  }
  double range[2];
  if (!this->groupDataRange(group, range))
  {
    qCritical().noquote() << tr("Cannot compute the data range of partition group %1.").arg(group);
    return;
  }
  this->setGroupCustomRange(group, range);
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::rescaleToCustomRange()
{
  const int group = this->selectedGroup();
  if (group < 0)
  {
    return;
  }
  // start from the current range of the group
  double range[2] = { 0, 1 };
  if (this->CachedCustomRange.size() == 2 * this->separatorCount())
  {
    range[0] = this->CachedCustomRange[2 * group].toDouble();
    range[1] = this->CachedCustomRange[2 * group + 1].toDouble();
  }
  else if (!this->groupDataRange(group, range))
  {
    range[0] = 0;
    range[1] = 1;
  }

  pqRescaleScalarRangeToCustomDialog dialog(pqCoreUtilities::mainWidget());
  dialog.setWindowTitle(tr("Set Range for Partition Group %1").arg(group));
  dialog.showOpacityControls(false);
  dialog.showLockControl(false);
  dialog.setRange(range[0], range[1]);
  this->connect(&dialog, &pqRescaleScalarRangeToCustomDialog::apply, this,
    [this, &dialog, group]()
    {
      const double customRange[2] = { dialog.minimum(), dialog.maximum() };
      this->setGroupCustomRange(group, customRange);
    });
  dialog.exec();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqPartitionEditorWidget::partitionIndices() const
{
  return this->CachedIndices;
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::setPartitionIndices(const QList<QVariant>& values)
{
  this->CachedIndices = values;
  this->renderList();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqPartitionEditorWidget::partitionOffsets() const
{
  return this->CachedOffsets;
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::setPartitionOffsets(const QList<QVariant>& values)
{
  this->CachedOffsets = values;
  this->renderList();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqPartitionEditorWidget::customRange() const
{
  return this->CachedCustomRange;
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::setCustomRange(const QList<QVariant>& values)
{
  this->CachedCustomRange = values;
  this->updateButtonStates();
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::componentsChanged()
{
  const QStringList key = this->componentsKey();
  const bool seriesEdited = this->seriesEdited();
  if (key != this->ComponentsKey)
  {
    const bool hadComponents = !this->ComponentsKey.isEmpty();
    // undo and redo restore the partition and custom ranges with the components
    pqUndoStack* undoStack = pqApplicationCore::instance()->getUndoStack();
    const bool undoRedo = undoStack && (undoStack->getInUndo() || undoStack->getInRedo());
    // a partition set with the components from Python or a state file is kept. A
    // representation applies its edits right away, so they are never pending; its
    // RadViz view sets the partition with the filter's components.
    const bool panelEdit = this->VisibleComponentsProperty
      ? ::HasPendingValue(this->VisibleComponentsProperty) ||
        ::HasPendingValue(this->arrayProperty())
      : !pqRadVizChartView::isUpdatingSeries(this->proxy());
    const bool clear = hadComponents && !undoRedo && panelEdit && seriesEdited;
    this->ComponentsKey = key;
    // the partition and custom ranges were set for the previous components
    if (clear && (!this->CachedIndices.isEmpty() || !this->CachedOffsets.isEmpty()))
    {
      this->CachedIndices.clear();
      this->CachedOffsets.clear();
      Q_EMIT this->partitionIndicesChanged();
      Q_EMIT this->partitionOffsetsChanged();
    }
    if (clear && !this->CachedCustomRange.isEmpty())
    {
      this->CachedCustomRange.clear();
      Q_EMIT this->customRangeChanged();
    }
  }
  this->renderList();
}

//-----------------------------------------------------------------------------
bool pqPartitionEditorWidget::seriesEdited()
{
  auto* svp = vtkSMStringVectorProperty::SafeDownCast(this->ComponentsProperty);
  if (!svp || this->ComponentsProperty->FindDomain<vtkSMEnumerationDomain>())
  {
    return true;
  }
  const QSet<QString> all = ::allNames(svp);
  const QStringList visibleList = ::visibleNames(svp);
  const QSet<QString> visible(visibleList.begin(), visibleList.end());
  // the series domain appends new series, visible by default: they are not edits.
  QSet<QString> listedVisible = visible;
  listedVisible.intersect(this->ListedSeries);
  const bool edited = listedVisible != this->ListedVisibleSeries;
  this->ListedSeries = all;
  this->ListedVisibleSeries = visible;
  return edited;
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::renderList()
{
  const int numComponents = this->numberOfComponents();

  QList<QVariant> indices = this->CachedIndices;
  QList<QVariant> offsets = this->CachedOffsets;
  if (!::isValidPartition(indices, offsets, numComponents))
  {
    // display-only fallback: identity order with a single trailing separator. The
    // properties are left untouched until the user interacts with the widget.
    indices.clear();
    offsets.clear();
    for (int i = 0; i < numComponents; ++i)
    {
      indices.append(i);
    }
    if (numComponents > 0)
    {
      offsets.append(numComponents);
    }
  }

  const QSignalBlocker blocker(this->ListWidget);
  this->ListWidget->clear();
  int offsetIdx = 0;
  int seen = 0;
  for (const QVariant& v : indices)
  {
    int value = v.toInt();
    auto* item = new QListWidgetItem(this->componentLabel(value), this->ListWidget);
    item->setData(Qt::UserRole, value);
    ++seen;
    if (offsetIdx < offsets.size() && offsets[offsetIdx].toInt() == seen)
    {
      auto* separator = new QListWidgetItem(this->ListWidget);
      ::styleSeparatorItem(this->ListWidget, separator);
      ++offsetIdx;
    }
  }

  this->updateListHeight();
  this->updateButtonStates();
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::commitList()
{
  QList<QVariant> indices;
  QList<QVariant> offsets;
  int seen = 0;
  for (int row = 0; row < this->ListWidget->count(); ++row)
  {
    QListWidgetItem* item = this->ListWidget->item(row);
    if (::isSeparator(item))
    {
      offsets.append(seen);
    }
    else
    {
      indices.append(item->data(Qt::UserRole));
      ++seen;
    }
  }

  bool indicesChanged = indices != this->CachedIndices;
  this->CachedIndices = indices;
  bool offsetsChanged = offsets != this->CachedOffsets;
  this->CachedOffsets = offsets;

  if (indicesChanged)
  {
    Q_EMIT this->partitionIndicesChanged();
  }
  if (offsetsChanged)
  {
    Q_EMIT this->partitionOffsetsChanged();
  }
  // the custom ranges were set for the previous groups
  if ((indicesChanged || offsetsChanged) && !this->CachedCustomRange.isEmpty())
  {
    this->CachedCustomRange.clear();
    Q_EMIT this->customRangeChanged();
  }
}

//-----------------------------------------------------------------------------
int pqPartitionEditorWidget::separatorCount() const
{
  int count = 0;
  for (int row = 0; row < this->ListWidget->count(); ++row)
  {
    if (::isSeparator(this->ListWidget->item(row)))
    {
      ++count;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
bool pqPartitionEditorWidget::canMoveUp(int row) const
{
  if (row <= 0 || row >= this->ListWidget->count())
  {
    return false;
  }
  if (::isSeparator(this->ListWidget->item(row)))
  {
    return false;
  }
  if (::isSeparator(this->ListWidget->item(row - 1)) &&
    ::isSeparator(this->ListWidget->item(row + 1)))
  {
    // alone in its group: moving it would leave the group empty.
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool pqPartitionEditorWidget::canMoveDown(int row) const
{
  const int count = this->ListWidget->count();
  // Rows count - 2 and count - 1 can never move down: count - 1 is the
  // fixed trailing separator, and moving count - 2 down would swap it into
  // that fixed separator's spot.
  if (row < 0 || row > count - 3)
  {
    return false;
  }
  if (::isSeparator(this->ListWidget->item(row)))
  {
    return false;
  }
  if (row == 0 && ::isSeparator(this->ListWidget->item(1)))
  {
    // Would place a separator before the first component.
    return false;
  }
  if (::isSeparator(this->ListWidget->item(row - 1)) &&
    ::isSeparator(this->ListWidget->item(row + 1)))
  {
    // alone in its group: moving it would leave the group empty.
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool pqPartitionEditorWidget::canAddSeparatorAfter(int row) const
{
  const int count = this->ListWidget->count();
  if (row < 0 || row >= count - 1)
  {
    // No component selected, or it is the fixed trailing separator.
    return false;
  }
  if (::isSeparator(this->ListWidget->item(row)))
  {
    return false;
  }
  if (::isSeparator(this->ListWidget->item(row + 1)))
  {
    // already a separator right after: another would create an empty group.
    return false;
  }
  return this->separatorCount() < this->numberOfComponents();
}

//-----------------------------------------------------------------------------
bool pqPartitionEditorWidget::canRemoveSeparatorAt(int row) const
{
  const int count = this->ListWidget->count();
  if (row < 0 || row >= count - 1)
  {
    // Not a row, or the fixed trailing separator: never removable.
    return false;
  }
  return ::isSeparator(this->ListWidget->item(row));
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::moveUp()
{
  int row = this->ListWidget->currentRow();
  if (!this->canMoveUp(row))
  {
    return;
  }
  QListWidgetItem* item = this->ListWidget->takeItem(row);
  this->ListWidget->insertItem(row - 1, item);
  this->ListWidget->setCurrentItem(item);
  this->commitList();
  this->updateButtonStates();
  this->ListWidget->setFocus();
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::moveDown()
{
  int row = this->ListWidget->currentRow();
  if (!this->canMoveDown(row))
  {
    return;
  }
  QListWidgetItem* item = this->ListWidget->takeItem(row);
  this->ListWidget->insertItem(row + 1, item);
  this->ListWidget->setCurrentItem(item);
  this->commitList();
  this->updateButtonStates();
  this->ListWidget->setFocus();
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::addSeparator()
{
  int row = this->ListWidget->currentRow();
  if (!this->canAddSeparatorAfter(row))
  {
    return;
  }
  auto* separator = new QListWidgetItem();
  this->ListWidget->insertItem(row + 1, separator);
  ::styleSeparatorItem(this->ListWidget, separator);
  this->updateListHeight();
  this->commitList();
  this->updateButtonStates();
  this->ListWidget->setFocus();
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::removeSeparator()
{
  int row = this->ListWidget->currentRow();
  if (!this->canRemoveSeparatorAt(row))
  {
    return;
  }
  delete this->ListWidget->takeItem(row);
  // The removed separator was the current item; select what took its place
  // so the arrow keys have a current item to move from.
  this->ListWidget->setCurrentRow(std::min(row, this->ListWidget->count() - 1));
  this->updateListHeight();
  this->commitList();
  this->updateButtonStates();
  this->ListWidget->setFocus();
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::updateListHeight()
{
  int height = 2 * this->ListWidget->frameWidth();
  const int numRows = std::min(this->ListWidget->count(), ::MAX_VISIBLE_ROWS);
  for (int row = 0; row < numRows; ++row)
  {
    height += this->ListWidget->sizeHintForRow(row);
  }
  this->ListWidget->setFixedHeight(std::max(height, this->ButtonsHeight));
}

//-----------------------------------------------------------------------------
void pqPartitionEditorWidget::updateButtonStates()
{
  int row = this->ListWidget->currentRow();
  this->UpButton->setEnabled(this->canMoveUp(row));
  this->DownButton->setEnabled(this->canMoveDown(row));
  this->AddSeparatorButton->setEnabled(this->canAddSeparatorAfter(row));
  this->RemoveSeparatorButton->setEnabled(this->canRemoveSeparatorAt(row));
  // rescaling needs the data ranges, unknown for series not found in the input
  const int group = this->selectedGroup();
  double range[2];
  this->RescaleButton->setEnabled(group >= 0 && this->groupDataRange(group, range));
  bool canSetCustomRange = group >= 0;
  if (canSetCustomRange && this->CachedCustomRange.size() != 2 * this->separatorCount())
  {
    for (int g = 0; g < this->separatorCount() && canSetCustomRange; ++g)
    {
      canSetCustomRange = this->groupDataRange(g, range);
    }
  }
  this->RescaleCustomButton->setEnabled(canSetCustomRange);
}
