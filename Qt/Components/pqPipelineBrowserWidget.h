// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPipelineBrowserWidget_h
#define pqPipelineBrowserWidget_h

#include "pqComponentsModule.h"
#include "pqFlatTreeView.h"

#include <QPointer>

class pqPipelineModel;
class pqPipelineAnnotationFilterModel;
class pqPipelineSource;
class pqOutputPort;
class pqView;
class QAction;
class QMenu;
class vtkPVSession;

/**
 * pqPipelineBrowserWidget is the widget for the pipeline  browser. This is a
 * replacement for pqPipelineBrowser.
 */
class PQCOMPONENTS_EXPORT pqPipelineBrowserWidget : public pqFlatTreeView
{
  Q_OBJECT
  typedef pqFlatTreeView Superclass;

public:
  pqPipelineBrowserWidget(QWidget* parent = nullptr);
  ~pqPipelineBrowserWidget() override;

  /**
   * Used to monitor the key press events in the tree view.
   * Returns True if the event should not be sent to the object.
   */
  bool eventFilter(QObject* object, QEvent* e) override;

  /**
   * Set the visibility of selected items.
   */
  void setSelectionVisibility(bool visible);

  /**
   * Set Annotation filter to use
   */
  void enableAnnotationFilter(const QString& annotationKey);

  /**
   * Disable any Annotation filter
   */
  void disableAnnotationFilter();

  /**
   * Choose wether Annotation filter should display matching or non-matching sources.
   * Default is matching.
   */
  void setAnnotationFilterMatching(bool matching);

  /**
   * Set Session filter to use
   */
  void enableSessionFilter(vtkPVSession* session);

  /**
   * Disable any Session filter
   */
  void disableSessionFilter();

  /**
   * Overload of pqFlatTreeView::setModel
   */
  void setModel(pqPipelineModel* model);

  /**
   * TODO document
   * @note Moved from proteced
   */
  QModelIndex pipelineModelIndex(const QModelIndex& index) const;
  const pqPipelineModel* getPipelineModel(const QModelIndex& index) const;

  /**
   * static method to sets the visibility of a pqOutputPort
   */
  static void setVisibility(bool visible, pqOutputPort* port);

  /**
   * Provides access to the context menu.
   */
  QMenu* contextMenu() const;

  /**
   * Provides access to the action toggling the visibility of the selected
   * items (see toggleSelectionVisibility()). Its default shortcut is `Space`,
   * active only when this widget has focus. It is meant to be shared by the
   * application menus and the context menu so that a single shortcut exists.
   */
  QAction* toggleVisibilityAction() const;

Q_SIGNALS:
  /**
   * Fired when the delete key is pressed.
   * Typically implies that the selected items need to be deleted.
   */
  void deleteKey();

public Q_SLOTS:
  /**
   * Set the active view. By default connected to
   * pqActiveObjects::viewChanged() so it keeps track of the active view.
   */
  void setActiveView(pqView*);

  /**
   * Toggle the visibility of selected items in the active view.
   * The new state is the opposite of the current visibility of the current
   * item (or of the first selected item with an output port if the current
   * item is not selected), and it is applied to all selected items, the same
   * way clicking the eye icon of a selected item does.
   */
  void toggleSelectionVisibility();

protected Q_SLOTS:
  void handleIndexClicked(const QModelIndex& index);
  void expandWithModelIndexTranslation(const QModelIndex&);
  void onActiveSourceChanged(pqPipelineSource* source);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * sets the visibility for items in the indices list.
   */
  void setVisibility(bool visible, const QModelIndexList& indices);

  /**
   * Returns the output port corresponding to the given index, if any.
   */
  pqOutputPort* getOutputPort(const QModelIndex& index) const;

  /**
   * Returns whether the given port is visible in the active view.
   */
  static bool isVisibleInActiveView(pqOutputPort* port);

  void contextMenuEvent(QContextMenuEvent* e) override;

  /**
   * Overridden to pass changed font to pqPipelineModel.
   */
  bool viewportEvent(QEvent* e) override;

  pqPipelineModel* PipelineModel;
  pqPipelineAnnotationFilterModel* FilteredPipelineModel;
  QPointer<QMenu> ContextMenu;
  QAction* ToggleVisibilityAction;

private:
  /**
   * Set up the current pqPipelineModel.
   */
  void configureModel();

  Q_DISABLE_COPY(pqPipelineBrowserWidget)
};

#endif
