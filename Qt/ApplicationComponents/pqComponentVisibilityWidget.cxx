// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqComponentVisibilityWidget.h"

#include "pqApplicationCore.h"
#include "pqUndoStack.h"
#include "pqWidgetUtilities.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMDocumentation.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QCoreApplication>
#include <QListWidget>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>

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
}

//-----------------------------------------------------------------------------
pqComponentVisibilityWidget::pqComponentVisibilityWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , VTKConnector(vtkEventQtSlotConnect::New())
{
  this->ListWidget = new QListWidget(this);
  this->ListWidget->setObjectName("ComponentList");
  this->ListWidget->setSelectionMode(QAbstractItemView::NoSelection);
  if (smgroup->GetDocumentation())
  {
    QString doc = QCoreApplication::translate(
      "ServerManagerXML", smgroup->GetDocumentation()->GetDescription());
    this->ListWidget->setToolTip(pqWidgetUtilities::formatTooltip(doc));
  }
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(this->ListWidget);

  this->connect(
    this->ListWidget, &QListWidget::itemChanged, this, &pqComponentVisibilityWidget::itemChanged);

  this->addPropertyLink(this, "visibleComponents", SIGNAL(visibleComponentsChanged()),
    smgroup->GetProperty("VisibleComponents"));

  this->ComponentsProperty = smgroup->GetProperty("Components");
  if (this->ComponentsProperty)
  {
    this->VTKConnector->Connect(
      this->ComponentsProperty, vtkCommand::DomainModifiedEvent, this, SLOT(componentsChanged()));
  }
  this->Names = this->componentNames();
  this->renderList();
}

//-----------------------------------------------------------------------------
pqComponentVisibilityWidget::~pqComponentVisibilityWidget()
{
  this->VTKConnector->Delete();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqComponentVisibilityWidget::visibleComponents() const
{
  return this->VisibleComponents;
}

//-----------------------------------------------------------------------------
void pqComponentVisibilityWidget::setVisibleComponents(const QList<QVariant>& indices)
{
  if (this->VisibleComponents != indices)
  {
    this->VisibleComponents = indices;
    this->renderList();
  }
}

//-----------------------------------------------------------------------------
QStringList pqComponentVisibilityWidget::componentNames() const
{
  QStringList names;
  auto* domain = this->ComponentsProperty
    ? this->ComponentsProperty->FindDomain<vtkSMEnumerationDomain>()
    : nullptr;
  if (!domain)
  {
    return names;
  }
  for (unsigned int i = 0; i < domain->GetNumberOfEntries(); ++i)
  {
    const char* name = domain->GetEntryText(i);
    names.append(
      name && name[0] ? tr("%1: %2").arg(i).arg(QString::fromUtf8(name)) : QString::number(i));
  }
  return names;
}

//-----------------------------------------------------------------------------
void pqComponentVisibilityWidget::renderList()
{
  QSignalBlocker blocker(this->ListWidget);
  this->ListWidget->clear();
  // like the filter, use all components if none of the indices is valid
  const bool showAll = std::none_of(this->VisibleComponents.begin(), this->VisibleComponents.end(),
    [this](const QVariant& index)
    { return index.toInt() >= 0 && index.toInt() < this->Names.size(); });
  for (int i = 0; i < this->Names.size(); ++i)
  {
    QListWidgetItem* item = new QListWidgetItem(this->Names[i], this->ListWidget);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    const bool visible = showAll || this->VisibleComponents.contains(i);
    item->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
  }
  // fit the rows, so the list does not scroll.
  const int rows = this->ListWidget->count();
  const int rowHeight = rows > 0 ? this->ListWidget->sizeHintForRow(0) : 0;
  this->ListWidget->setFixedHeight(rows * rowHeight + 2 * this->ListWidget->frameWidth());
}

//-----------------------------------------------------------------------------
void pqComponentVisibilityWidget::itemChanged(QListWidgetItem* item)
{
  QList<QVariant> visible;
  for (int i = 0; i < this->ListWidget->count(); ++i)
  {
    if (this->ListWidget->item(i)->checkState() == Qt::Checked)
    {
      visible.append(i);
    }
  }
  if (visible.size() < MIN_VISIBLE_COMPONENTS)
  {
    QSignalBlocker blocker(this->ListWidget);
    item->setCheckState(Qt::Checked);
    return;
  }
  if (visible.size() == this->ListWidget->count())
  {
    visible.clear();
  }
  if (this->VisibleComponents != visible)
  {
    this->VisibleComponents = visible;
    Q_EMIT this->visibleComponentsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqComponentVisibilityWidget::componentsChanged()
{
  QStringList names = this->componentNames();
  if (names == this->Names)
  {
    return;
  }
  const bool hadComponents = !this->Names.isEmpty();
  this->Names = names;
  // undo and redo restore the visible components with the components
  pqUndoStack* undoStack = pqApplicationCore::instance()->getUndoStack();
  const bool undoRedo = undoStack && (undoStack->getInUndo() || undoStack->getInRedo());
  // visible components set with the array from Python or a state file are kept.
  auto* domain = this->ComponentsProperty->FindDomain<vtkSMEnumerationDomain>();
  const bool panelEdit = domain && ::HasPendingValue(domain->GetRequiredProperty("ArraySelection"));
  // indices of other components would select the wrong ones: show all.
  if (hadComponents && !undoRedo && panelEdit && !this->VisibleComponents.isEmpty())
  {
    this->VisibleComponents.clear();
    this->renderList();
    Q_EMIT this->visibleComponentsChanged();
    return;
  }
  this->renderList();
}
