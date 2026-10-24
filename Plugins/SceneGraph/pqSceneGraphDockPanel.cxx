// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSceneGraphDockPanel.h"
#include "ui_pqSceneGraphDockPanel.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDoubleLineEdit.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqSceneGraphModel.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSceneGraphNodeProxy.h"
#include "vtkSMSceneGraphProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkWeakPointer.h"

#include <QInputDialog>
#include <QItemSelectionModel>
#include <QSignalBlocker>
#include <QTimer>

#include <cstring>
#include <map>
#include <set>
#include <vector>

//-----------------------------------------------------------------------------
class pqSceneGraphDockPanel::pqInternals : public Ui::SceneGraphPanel
{
public:
  pqSceneGraphModel* Model = nullptr;

  // Parallel to graphCombo's items.
  std::vector<vtkWeakPointer<vtkSMSceneGraphProxy>> Graphs;

  vtkWeakPointer<vtkSMSessionProxyManager> ProxyManager;
  unsigned long RegisterTag = 0;
  unsigned long UnRegisterTag = 0;
  QTimer GraphRefreshTimer;

  // What to reselect and re-expand once the model has rebuilt. The selection
  // and expansion saved as the reset began, unless an action here asked for
  // a particular node (e.g. a freshly created child) to be selected instead.
  vtkWeakPointer<vtkSMSceneGraphNodeProxy> Selected;
  vtkWeakPointer<vtkSMSceneGraphNodeProxy> PendingSelection;
  bool PendingEdit = false;
  std::set<vtkSMSceneGraphNodeProxy*> Expanded;
  bool FirstBuild = true;

  // Set while the editors are being filled from the graph, so their change
  // signals are not mistaken for user edits.
  bool Updating = false;

  static QString nameOf(vtkSMProxy* proxy, const char* group)
  {
    auto* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    if (auto* item = smmodel->findItem<pqProxy*>(proxy))
    {
      return item->getSMName();
    }
    if (auto* pxm = proxy ? proxy->GetSessionProxyManager() : nullptr)
    {
      return QString::fromUtf8(pxm->GetProxyName(group, proxy));
    }
    return QString();
  }
};

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::constructor()
{
  this->setWindowTitle(tr("Scene Graph"));
  auto* container = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(container);
  this->setWidget(container);

  auto& ui = *this->Internals;
  ui.Model = new pqSceneGraphModel(this);
  ui.treeView->setModel(ui.Model);

  // These two lists rarely hold more than a couple of entries, so cap them at
  // about three rows (based on the font size), allowing them to scroll. The
  // tree view then takes whatever height is left.
  for (QListWidget* list : { ui.sourcesList, ui.viewsList })
  {
    const QMargins margins = list->contentsMargins();
    const int rows = 3;
    list->setMaximumHeight(rows * list->fontMetrics().height() + margins.top() + margins.bottom() +
      2 * list->frameWidth() + rows * 2);
    list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  }

  ui.GraphRefreshTimer.setSingleShot(true);
  ui.GraphRefreshTimer.setInterval(0);
  QObject::connect(
    &ui.GraphRefreshTimer, &QTimer::timeout, this, &pqSceneGraphDockPanel::refreshGraphs);

  QObject::connect(ui.graphCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &pqSceneGraphDockPanel::graphSelected);
  QObject::connect(
    ui.newGraphButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::newGraph);
  QObject::connect(
    ui.renameGraphButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::renameGraph);
  QObject::connect(
    ui.deleteGraphButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::deleteGraph);

  QObject::connect(
    ui.addChildButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::addChild);
  QObject::connect(
    ui.renameButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::renameNode);
  QObject::connect(
    ui.deleteNodeButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::deleteNode);
  QObject::connect(
    ui.deleteSubtreeButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::deleteSubtree);

  for (auto* edit : { ui.translateX, ui.translateY, ui.translateZ })
  {
    QObject::connect(edit, &pqLineEdit::textChangedAndEditingFinished, this,
      &pqSceneGraphDockPanel::translationEdited);
  }

  QObject::connect(
    ui.bindSourceButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::bindActiveSource);
  QObject::connect(
    ui.unbindSourceButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::unbindSource);
  QObject::connect(
    ui.targetViewButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::targetActiveView);
  QObject::connect(
    ui.untargetViewButton, &QPushButton::clicked, this, &pqSceneGraphDockPanel::untargetView);

  QObject::connect(ui.treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
    &pqSceneGraphDockPanel::refreshEditors);
  QObject::connect(ui.Model, &QAbstractItemModel::modelAboutToBeReset, this,
    &pqSceneGraphDockPanel::saveTreeState);
  QObject::connect(
    ui.Model, &pqSceneGraphModel::rebuilt, this, &pqSceneGraphDockPanel::restoreTreeState);
  QObject::connect(
    ui.Model, &pqSceneGraphModel::contentChanged, this, &pqSceneGraphDockPanel::refreshEditors);
  QObject::connect(
    ui.Model, &pqSceneGraphModel::message, this, &pqSceneGraphDockPanel::showMessage);

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::serverChanged, this,
    &pqSceneGraphDockPanel::serverChanged);
  this->serverChanged(pqActiveObjects::instance().activeServer());
}

//-----------------------------------------------------------------------------
pqSceneGraphDockPanel::~pqSceneGraphDockPanel()
{
  this->watchProxyManager(nullptr);
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::watchProxyManager(vtkSMSessionProxyManager* pxm)
{
  auto& ui = *this->Internals;
  if (ui.ProxyManager)
  {
    ui.ProxyManager->RemoveObserver(ui.RegisterTag);
    ui.ProxyManager->RemoveObserver(ui.UnRegisterTag);
  }
  ui.ProxyManager = pxm;
  ui.RegisterTag = ui.UnRegisterTag = 0;
  if (pxm)
  {
    ui.RegisterTag = pxm->AddObserver(
      vtkCommand::RegisterEvent, this, &pqSceneGraphDockPanel::onGraphRegistration);
    ui.UnRegisterTag = pxm->AddObserver(
      vtkCommand::UnRegisterEvent, this, &pqSceneGraphDockPanel::onGraphRegistration);
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::onGraphRegistration(vtkObject*, unsigned long, void* callData)
{
  auto* info = static_cast<vtkSMProxyManager::RegisteredProxyInformation*>(callData);
  if (!info || !(info->Type & vtkSMProxyManager::RegisteredProxyInformation::PROXY) ||
    !info->GroupName ||
    std::strcmp(info->GroupName, vtkSMSceneGraphProxy::GetSceneGraphGroup()) != 0)
  {
    return;
  }
  // Deferred because the event seems to fire from inside the registration,
  // before the graph is finished.
  this->Internals->GraphRefreshTimer.start();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::serverChanged(pqServer* server)
{
  this->watchProxyManager(server ? server->proxyManager() : nullptr);
  this->refreshGraphs();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::refreshGraphs()
{
  auto& ui = *this->Internals;
  vtkSMSceneGraphProxy* previous = this->graph();

  std::vector<vtkSMSceneGraphProxy*> graphs;
  if (ui.ProxyManager)
  {
    vtkNew<vtkCollection> collection;
    vtkSMSceneGraphProxy::GetSceneGraphs(ui.ProxyManager, collection);
    for (int cc = 0; cc < collection->GetNumberOfItems(); ++cc)
    {
      if (auto* g = vtkSMSceneGraphProxy::SafeDownCast(collection->GetItemAsObject(cc)))
      {
        graphs.push_back(g);
      }
    }
  }

  // A graph that was not listed before was either just created here, or just loaded
  // from a statefile and is likely the one the user is about to want. Otherwise stay
  // on the graph already shown.
  vtkSMSceneGraphProxy* newest = nullptr;
  for (auto* g : graphs)
  {
    bool known = false;
    for (const auto& old : ui.Graphs)
    {
      known = known || old == g;
    }
    if (!known)
    {
      newest = g;
    }
  }
  vtkSMSceneGraphProxy* wanted = newest ? newest : previous;

  // Registration names are not unique across a statefile load, so a repeated
  // name is told apart by its global id.
  std::map<QString, int> nameCounts;
  std::vector<QString> names;
  for (auto* g : graphs)
  {
    names.push_back(QString::fromUtf8(
      ui.ProxyManager->GetProxyName(vtkSMSceneGraphProxy::GetSceneGraphGroup(), g)));
    ++nameCounts[names.back()];
  }

  {
    // Make sure we don't emit currentIndexChanged until we're ready
    const QSignalBlocker blocker(ui.graphCombo);
    ui.graphCombo->clear();
    ui.Graphs.clear();
    int select = graphs.empty() ? -1 : 0;
    for (size_t cc = 0; cc < graphs.size(); ++cc)
    {
      if (graphs[cc] == wanted)
      {
        select = static_cast<int>(cc);
      }
      ui.Graphs.emplace_back(graphs[cc]);
      QString label = names[cc];
      if (nameCounts[label] > 1)
      {
        label += QString(" (id %1)").arg(graphs[cc]->GetGlobalID());
      }
      ui.graphCombo->addItem(label);
    }
    ui.graphCombo->setCurrentIndex(select);
  }
  this->graphSelected(ui.graphCombo->currentIndex());
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::graphSelected(int index)
{
  auto& ui = *this->Internals;
  vtkSMSceneGraphProxy* g =
    (index >= 0 && index < static_cast<int>(ui.Graphs.size())) ? ui.Graphs[index] : nullptr;
  ui.renameGraphButton->setEnabled(g != nullptr);
  ui.deleteGraphButton->setEnabled(g != nullptr);
  ui.nodesGroup->setEnabled(g != nullptr);
  ui.viewsGroup->setEnabled(g != nullptr);
  if (g != ui.Model->graph())
  {
    ui.Selected = nullptr;
    ui.Expanded.clear();
    ui.FirstBuild = true;
    ui.Model->setGraph(g);
  }
  this->refreshEditors();
}

//-----------------------------------------------------------------------------
vtkSMSceneGraphProxy* pqSceneGraphDockPanel::graph() const
{
  return this->Internals->Model->graph();
}

//-----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* pqSceneGraphDockPanel::currentNode() const
{
  auto& ui = *this->Internals;
  return ui.Model->node(ui.treeView->currentIndex());
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::selectNode(vtkSMSceneGraphNodeProxy* node)
{
  auto& ui = *this->Internals;
  const QModelIndex index = ui.Model->indexOf(node);
  if (index.isValid())
  {
    ui.treeView->setCurrentIndex(index);
    ui.treeView->scrollTo(index);
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::newGraph()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  if (!ui.ProxyManager)
  {
    this->showMessage(tr("No active server."));
    return;
  }
  const char* group = vtkSMSceneGraphProxy::GetSceneGraphGroup();
  const QString suggested =
    QString::fromStdString(ui.ProxyManager->GetUniqueProxyName(group, "SceneGraph", true));
  bool accepted = false;
  const QString name = QInputDialog::getText(
    this, tr("New Scene Graph"), tr("Name:"), QLineEdit::Normal, suggested, &accepted)
                         .trimmed();
  if (!accepted)
  {
    return;
  }
  vtkSMSceneGraphProxy* g = vtkSMSceneGraphProxy::CreateSceneGraph(
    ui.ProxyManager, name.isEmpty() ? nullptr : name.toUtf8().data());
  if (!g)
  {
    this->showMessage(tr("Could not create a scenegraph."));
    return;
  }
  // Refresh now rather than on the deferred registration event, so the new
  // graph is the one shown when the click returns.
  this->refreshGraphs();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::renameGraph()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  if (!g || !ui.ProxyManager)
  {
    return;
  }
  const char* group = vtkSMSceneGraphProxy::GetSceneGraphGroup();
  const QString current = QString::fromUtf8(ui.ProxyManager->GetProxyName(group, g));
  bool accepted = false;
  const QString name = QInputDialog::getText(
    this, tr("Rename Scene Graph"), tr("Name:"), QLineEdit::Normal, current, &accepted)
                         .trimmed();
  if (!accepted || name.isEmpty() || name == current)
  {
    return;
  }
  if (!vtkSMSceneGraphProxy::RenameSceneGraph(ui.ProxyManager, g, name.toUtf8().data()))
  {
    this->showMessage(tr("Could not rename the scenegraph."));
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::deleteGraph()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  if (!g || !ui.ProxyManager)
  {
    return;
  }
  // Deleting the graph puts back whatever representations it was affecting, so
  // here we render the deleted graph's targeted views, so the effect shows.
  vtkNew<vtkCollection> views;
  g->GetTargetViews(views);
  ui.Model->setGraph(nullptr);
  vtkSMSceneGraphProxy::DeleteSceneGraph(ui.ProxyManager, g);
  auto* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  for (int cc = 0; cc < views->GetNumberOfItems(); ++cc)
  {
    if (auto* view =
          smmodel->findItem<pqView*>(vtkSMProxy::SafeDownCast(views->GetItemAsObject(cc))))
    {
      view->render();
    }
  }
  this->refreshGraphs();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::addChild()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  if (!g)
  {
    return;
  }
  vtkSMSceneGraphNodeProxy* parent = this->currentNode();
  if (!parent)
  {
    parent = g->GetRoot();
  }
  if (auto* child = g->CreateChild(parent, "Node"))
  {
    // Select it and open its name for editing once the tree has caught up.
    ui.PendingSelection = child;
    ui.PendingEdit = true;
  }
  else
  {
    this->showMessage(tr("Could not create a node."));
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::renameNode()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  const QModelIndex index = ui.treeView->currentIndex();
  if (index.isValid())
  {
    ui.treeView->edit(index);
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::deleteNode()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  auto* node = this->currentNode();
  if (!g || !node)
  {
    return;
  }
  if (!g->DeleteNode(node))
  {
    this->showMessage(tr("The root cannot be deleted."));
    return;
  }
  // Deleting a node causes it's children to have a new cumulative transform,
  // make sure that is visible on the screen.
  this->applyAndRender();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::deleteSubtree()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  auto* node = this->currentNode();
  if (!g || !node)
  {
    return;
  }
  if (!g->RemoveSubtree(node))
  {
    this->showMessage(tr("The root cannot be deleted."));
    return;
  }
  this->applyAndRender();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::translationEdited()
{
  auto& ui = *this->Internals;
  auto* g = this->graph();
  auto* node = this->currentNode();
  if (ui.Updating || !g || !node)
  {
    return;
  }
  // Currently we just provide a view/edit onto the translation part of the
  // LocalTransform proxy property, so here we only replace that part of the
  // matrix.
  vtkNew<vtkMatrix4x4> matrix;
  g->GetLocalTransform(node, matrix);
  matrix->SetElement(0, 3, ui.translateX->text().toDouble());
  matrix->SetElement(1, 3, ui.translateY->text().toDouble());
  matrix->SetElement(2, 3, ui.translateZ->text().toDouble());
  g->SetLocalTransform(node, matrix);
  this->applyAndRender();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::bindActiveSource()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  auto* node = this->currentNode();
  if (!g || !node)
  {
    this->showMessage(tr("Select a node to bind to."));
    return;
  }
  pqOutputPort* port = pqActiveObjects::instance().activePort();
  auto* source = port ? vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy()) : nullptr;
  if (!source)
  {
    this->showMessage(tr("No active source."));
    return;
  }
  if (!g->AddSource(node, source, port->getPortNumber()))
  {
    this->showMessage(QString::fromUtf8(g->GetErrorMessage()));
    return;
  }
  this->applyAndRender();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::unbindSource()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  auto* node = this->currentNode();
  if (!g || !node)
  {
    return;
  }
  int row = ui.sourcesList->currentRow();
  if (row < 0 && ui.sourcesList->count() == 1)
  {
    row = 0; // the only one there is
  }
  if (row < 0)
  {
    this->showMessage(tr("Select a bound source to unbind."));
    return;
  }
  auto* source = g->GetSource(node, static_cast<unsigned int>(row));
  const int port = g->GetSourcePort(node, static_cast<unsigned int>(row));
  if (source && g->RemoveSource(node, source, port))
  {
    this->applyAndRender();
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::targetActiveView()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  pqView* view = pqActiveObjects::instance().activeView();
  if (!g || !view)
  {
    this->showMessage(tr("No active view."));
    return;
  }
  if (!g->AddTargetView(view->getViewProxy()))
  {
    this->showMessage(QString::fromUtf8(g->GetErrorMessage()));
    return;
  }
  this->applyAndRender();
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::untargetView()
{
  auto& ui = *this->Internals;
  ui.statusLabel->clear();
  auto* g = this->graph();
  if (!g)
  {
    return;
  }
  int row = ui.viewsList->currentRow();
  if (row < 0 && ui.viewsList->count() == 1)
  {
    row = 0;
  }
  if (row < 0)
  {
    this->showMessage(tr("Select a target view to remove."));
    return;
  }
  vtkNew<vtkCollection> views;
  g->GetTargetViews(views);
  auto* view = vtkSMViewProxy::SafeDownCast(views->GetItemAsObject(row));
  if (view && g->RemoveTargetView(view))
  {
    // The view that was dropped is the one that needs a render.
    auto* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    if (auto* pqview = smmodel->findItem<pqView*>(view))
    {
      pqview->render();
    }
    this->applyAndRender();
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::applyAndRender()
{
  auto* g = this->graph();
  if (!g)
  {
    return;
  }
  g->TraverseAndApply();
  vtkNew<vtkCollection> views;
  g->GetTargetViews(views);
  auto* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  for (int cc = 0; cc < views->GetNumberOfItems(); ++cc)
  {
    if (auto* view =
          smmodel->findItem<pqView*>(vtkSMProxy::SafeDownCast(views->GetItemAsObject(cc))))
    {
      view->render();
    }
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::saveTreeState()
{
  auto& ui = *this->Internals;
  ui.Selected = this->currentNode();
  ui.Expanded.clear();
  std::vector<QModelIndex> pending{ QModelIndex() };
  while (!pending.empty())
  {
    const QModelIndex parent = pending.back();
    pending.pop_back();
    for (int row = 0, count = ui.Model->rowCount(parent); row < count; ++row)
    {
      const QModelIndex child = ui.Model->index(row, 0, parent);
      if (ui.treeView->isExpanded(child))
      {
        if (auto* node = ui.Model->node(child))
        {
          ui.Expanded.insert(node);
        }
      }
      pending.push_back(child);
    }
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::restoreTreeState()
{
  auto& ui = *this->Internals;
  if (ui.FirstBuild)
  {
    ui.FirstBuild = false;
    ui.treeView->expandAll();
  }
  for (auto* node : ui.Expanded)
  {
    const QModelIndex index = ui.Model->indexOf(node);
    if (index.isValid())
    {
      ui.treeView->expand(index);
    }
  }
  vtkSMSceneGraphNodeProxy* select = ui.PendingSelection ? ui.PendingSelection : ui.Selected;
  const bool edit = ui.PendingEdit && ui.PendingSelection;
  ui.PendingSelection = nullptr;
  ui.PendingEdit = false;
  if (select)
  {
    // Expanding the parent chain is what scrollTo() cannot do on its own.
    for (QModelIndex index = ui.Model->indexOf(select).parent(); index.isValid();
         index = index.parent())
    {
      ui.treeView->expand(index);
    }
    this->selectNode(select);
  }
  this->refreshEditors();
  if (edit)
  {
    ui.treeView->edit(ui.treeView->currentIndex());
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::refreshEditors()
{
  auto& ui = *this->Internals;
  auto* g = this->graph();
  auto* node = this->currentNode();
  ui.Updating = true;

  const bool haveNode = g && node;
  const bool isRoot = haveNode && node == g->GetRoot();
  ui.renameButton->setEnabled(haveNode);
  ui.deleteNodeButton->setEnabled(haveNode && !isRoot);
  ui.deleteSubtreeButton->setEnabled(haveNode && !isRoot);
  ui.transformGroup->setEnabled(haveNode);
  ui.sourcesGroup->setEnabled(haveNode);

  vtkNew<vtkMatrix4x4> matrix;
  if (haveNode)
  {
    g->GetLocalTransform(node, matrix);
  }
  ui.translateX->setTextAndResetCursor(QString::number(matrix->GetElement(0, 3), 'g', 15));
  ui.translateY->setTextAndResetCursor(QString::number(matrix->GetElement(1, 3), 'g', 15));
  ui.translateZ->setTextAndResetCursor(QString::number(matrix->GetElement(2, 3), 'g', 15));

  ui.sourcesList->clear();
  if (haveNode)
  {
    for (unsigned int cc = 0, n = g->GetNumberOfSources(node); cc < n; ++cc)
    {
      ui.sourcesList->addItem(QString("%1 : port %2")
                                .arg(pqInternals::nameOf(g->GetSource(node, cc), "sources"))
                                .arg(g->GetSourcePort(node, cc)));
    }
  }

  ui.viewsList->clear();
  if (g)
  {
    vtkNew<vtkCollection> views;
    g->GetTargetViews(views);
    for (int cc = 0; cc < views->GetNumberOfItems(); ++cc)
    {
      ui.viewsList->addItem(
        pqInternals::nameOf(vtkSMProxy::SafeDownCast(views->GetItemAsObject(cc)), "views"));
    }
  }

  ui.Updating = false;
}

//-----------------------------------------------------------------------------
void pqSceneGraphDockPanel::showMessage(const QString& text)
{
  this->Internals->statusLabel->setText(text);
}
