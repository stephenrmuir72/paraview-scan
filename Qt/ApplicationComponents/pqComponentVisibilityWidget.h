// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqComponentVisibilityWidget_h
#define pqComponentVisibilityWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QList>
#include <QStringList>
#include <QVariant>

class QListWidget;
class QListWidgetItem;
class vtkEventQtSlotConnect;
class vtkSMPropertyGroup;

/**
 * pqComponentVisibilityWidget is the custom property-group widget for the
 * `VisibleComponents` property of the `MultiVariateToRGBA` filter (see
 * vtkMultiVariateToRGBA::AddVisibleVariableIndex). It shows one checkbox per
 * component of the selected array, and sets `VisibleComponents` to the
 * indices of the checked components, or to an empty list (all components)
 * when every component is checked.
 *
 * The property group must provide two functions: "VisibleComponents", the
 * indices, and "Components", a property carrying a NumberOfComponentsDomain
 * (the filter's hidden PartitionNumberOfComponents property) that gives the
 * number of components and their names.
 *
 * At least MIN_VISIBLE_COMPONENTS components stay checked. When the
 * components change (e.g. another array is selected), all components become
 * visible again.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqComponentVisibilityWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> visibleComponents READ visibleComponents WRITE setVisibleComponents
      NOTIFY visibleComponentsChanged)
  typedef pqPropertyWidget Superclass;

public:
  pqComponentVisibilityWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = nullptr);
  ~pqComponentVisibilityWidget() override;

  /**
   * Smallest number of components that can be left visible.
   */
  static constexpr int MIN_VISIBLE_COMPONENTS = 3;

  ///@{
  /**
   * Indices of the visible components, empty when all are visible.
   */
  QList<QVariant> visibleComponents() const;
  void setVisibleComponents(const QList<QVariant>& indices);
  ///@}

Q_SIGNALS:
  void visibleComponentsChanged();

private Q_SLOTS:
  void itemChanged(QListWidgetItem* item);
  void componentsChanged();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqComponentVisibilityWidget)

  /**
   * Names of the components, from the "Components" property's domain.
   */
  QStringList componentNames() const;
  /**
   * Rebuilds the list from componentNames() and VisibleComponents.
   */
  void renderList();

  QListWidget* ListWidget;
  vtkSMProperty* ComponentsProperty = nullptr;
  vtkEventQtSlotConnect* VTKConnector;
  QList<QVariant> VisibleComponents;
  QStringList Names;
};

#endif
