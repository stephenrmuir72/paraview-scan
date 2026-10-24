// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqSceneGraphDockPanel
 * @brief   dock panel for building a scenegraph by hand
 *
 * Provide UI to create, delete, and rename scenegraphs, as well as build
 * the tree, rename nodes, reparent nodes, bind pipeline sources, and target
 * active views.
 */
#ifndef pqSceneGraphDockPanel_h
#define pqSceneGraphDockPanel_h

#include <QDockWidget>

class pqServer;
class vtkObject;
class vtkSMSceneGraphNodeProxy;
class vtkSMSceneGraphProxy;
class vtkSMSessionProxyManager;

class pqSceneGraphDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  pqSceneGraphDockPanel(
    const QString& title, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{})
    : Superclass(title, parent, flags)
  {
    this->constructor();
  }
  pqSceneGraphDockPanel(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{})
    : Superclass(parent, flags)
  {
    this->constructor();
  }
  ~pqSceneGraphDockPanel() override;

private Q_SLOTS:
  void serverChanged(pqServer* server);
  void refreshGraphs();
  void graphSelected(int index);
  void newGraph();
  void renameGraph();
  void deleteGraph();

  void addChild();
  void renameNode();
  void deleteNode();
  void deleteSubtree();

  void translationEdited();

  void bindActiveSource();
  void unbindSource();
  void targetActiveView();
  void untargetView();

  void saveTreeState();
  void restoreTreeState();
  void refreshEditors();
  void showMessage(const QString& text);

private:
  Q_DISABLE_COPY(pqSceneGraphDockPanel)

  void constructor();
  vtkSMSceneGraphProxy* graph() const;
  vtkSMSceneGraphNodeProxy* currentNode() const;
  void selectNode(vtkSMSceneGraphNodeProxy* node);
  void applyAndRender();
  void watchProxyManager(vtkSMSessionProxyManager* pxm);
  void onGraphRegistration(vtkObject* caller, unsigned long eventId, void* callData);

  class pqInternals;
  pqInternals* Internals;
};

#endif
