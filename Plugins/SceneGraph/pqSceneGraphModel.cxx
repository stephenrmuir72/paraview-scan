// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSceneGraphModel.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSceneGraphNodeProxy.h"
#include "vtkSMSceneGraphProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

#include <QMimeData>
#include <QTimer>

#include <cstring>
#include <set>
#include <unordered_map>
#include <vector>

namespace
{
const char* NODE_MIME_TYPE = "application/x-paraview-scenegraph-node";
}

//-----------------------------------------------------------------------------
class pqSceneGraphModel::pqInternals
{
public:
  struct Item
  {
    Item* Parent = nullptr;
    int Row = 0;
    std::vector<std::unique_ptr<Item>> Children;
    vtkWeakPointer<vtkSMSceneGraphNodeProxy> Node;
    unsigned long ObserverTag = 0;
    QString Name;
  };

  // Keep a hidden item above the "World" root, so the root is an ordinary row.
  Item Top;
  std::unordered_map<vtkSMSceneGraphNodeProxy*, Item*> Lookup;

  vtkWeakPointer<vtkSMSceneGraphProxy> Graph;
  unsigned long GraphObserverTag = 0;
  vtkWeakPointer<vtkSMSessionProxyManager> ProxyManager;
  unsigned long RegisterTag = 0;
  unsigned long UnRegisterTag = 0;

  QTimer Timer;
  bool PendingRebuild = false;
  bool PendingContent = false;

  static Item* item(const QModelIndex& index)
  {
    return index.isValid() ? static_cast<Item*>(index.internalPointer()) : nullptr;
  }

  // Returns true if candidate is ancestor itself or lies below it
  static bool isSameOrBelow(const Item* candidate, const Item* ancestor)
  {
    for (const Item* cur = candidate; cur; cur = cur->Parent)
    {
      if (cur == ancestor)
      {
        return true;
      }
    }
    return false;
  }

  // Returns the item for the node whose global ID is stored in the drag
  // data, or nullptr if that node is not in this model.
  Item* draggedItem(const QMimeData* data) const
  {
    if (!this->Graph)
    {
      return nullptr;
    }
    const vtkTypeUInt32 id = data->data(NODE_MIME_TYPE).toUInt();
    auto* node =
      vtkSMSceneGraphNodeProxy::SafeDownCast(this->Graph->GetSession()->GetRemoteObject(id));
    auto it = this->Lookup.find(node);
    return it == this->Lookup.end() ? nullptr : it->second;
  }

  void unwatchGraph()
  {
    if (this->Graph && this->GraphObserverTag)
    {
      this->Graph->RemoveObserver(this->GraphObserverTag);
    }
    if (this->ProxyManager)
    {
      this->ProxyManager->RemoveObserver(this->RegisterTag);
      this->ProxyManager->RemoveObserver(this->UnRegisterTag);
    }
    this->Graph = nullptr;
    this->GraphObserverTag = 0;
    this->ProxyManager = nullptr;
    this->RegisterTag = this->UnRegisterTag = 0;
  }

  void detachObservers()
  {
    for (auto& entry : this->Lookup)
    {
      Item* item = entry.second;
      if (item->Node && item->ObserverTag)
      {
        item->Node->RemoveObserver(item->ObserverTag);
      }
    }
    this->Lookup.clear();
    this->Top.Children.clear();
  }
};

//-----------------------------------------------------------------------------
pqSceneGraphModel::pqSceneGraphModel(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals())
{
  this->Internals->Timer.setSingleShot(true);
  this->Internals->Timer.setInterval(0);
  QObject::connect(&this->Internals->Timer, &QTimer::timeout, this, &pqSceneGraphModel::flush);
}

//-----------------------------------------------------------------------------
pqSceneGraphModel::~pqSceneGraphModel()
{
  this->Internals->Timer.stop();
  this->Internals->unwatchGraph();
  this->Internals->detachObservers();
}

//-----------------------------------------------------------------------------
void pqSceneGraphModel::setGraph(vtkSMSceneGraphProxy* graph)
{
  auto& internals = *this->Internals;
  internals.unwatchGraph();

  internals.Graph = graph;
  if (graph)
  {
    internals.GraphObserverTag = graph->AddObserver(
      vtkCommand::PropertyModifiedEvent, this, &pqSceneGraphModel::onGraphProperty);
    if (auto* pxm = graph->GetSessionProxyManager())
    {
      internals.ProxyManager = pxm;
      internals.RegisterTag =
        pxm->AddObserver(vtkCommand::RegisterEvent, this, &pqSceneGraphModel::onRegistration);
      internals.UnRegisterTag =
        pxm->AddObserver(vtkCommand::UnRegisterEvent, this, &pqSceneGraphModel::onRegistration);
    }
  }

  internals.PendingRebuild = internals.PendingContent = false;
  internals.Timer.stop();
  this->rebuild();
}

//-----------------------------------------------------------------------------
vtkSMSceneGraphProxy* pqSceneGraphModel::graph() const
{
  return this->Internals->Graph;
}

//-----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy* pqSceneGraphModel::node(const QModelIndex& index) const
{
  auto* item = pqInternals::item(index);
  return item ? item->Node.GetPointer() : nullptr;
}

//-----------------------------------------------------------------------------
QModelIndex pqSceneGraphModel::indexOf(vtkSMSceneGraphNodeProxy* node) const
{
  auto it = this->Internals->Lookup.find(node);
  if (it == this->Internals->Lookup.end())
  {
    return QModelIndex();
  }
  return this->createIndex(it->second->Row, 0, it->second);
}

//-----------------------------------------------------------------------------
QModelIndex pqSceneGraphModel::index(int row, int column, const QModelIndex& parentIndex) const
{
  auto* parentItem = parentIndex.isValid() ? pqInternals::item(parentIndex) : &this->Internals->Top;
  if (column != 0 || row < 0 || row >= static_cast<int>(parentItem->Children.size()))
  {
    return QModelIndex();
  }
  return this->createIndex(row, column, parentItem->Children[row].get());
}

//-----------------------------------------------------------------------------
QModelIndex pqSceneGraphModel::parent(const QModelIndex& index) const
{
  auto* item = pqInternals::item(index);
  if (!item || !item->Parent || item->Parent == &this->Internals->Top)
  {
    return QModelIndex();
  }
  return this->createIndex(item->Parent->Row, 0, item->Parent);
}

//-----------------------------------------------------------------------------
int pqSceneGraphModel::rowCount(const QModelIndex& parentIndex) const
{
  if (parentIndex.isValid() && parentIndex.column() != 0)
  {
    return 0;
  }
  auto* parentItem = parentIndex.isValid() ? pqInternals::item(parentIndex) : &this->Internals->Top;
  return static_cast<int>(parentItem->Children.size());
}

//-----------------------------------------------------------------------------
int pqSceneGraphModel::columnCount(const QModelIndex&) const
{
  return 1;
}

//-----------------------------------------------------------------------------
QVariant pqSceneGraphModel::data(const QModelIndex& index, int role) const
{
  auto* item = pqInternals::item(index);
  if (!item)
  {
    return QVariant();
  }
  switch (role)
  {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return item->Name;
    default:
      return QVariant();
  }
}

//-----------------------------------------------------------------------------
bool pqSceneGraphModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  auto* item = pqInternals::item(index);
  auto* graph = this->Internals->Graph.GetPointer();
  if (!item || !item->Node || !graph || role != Qt::EditRole)
  {
    return false;
  }
  const QString name = value.toString();
  if (name.isEmpty() || name == item->Name)
  {
    return false;
  }
  if (!graph->RenameNode(item->Node, name.toUtf8().data()))
  {
    Q_EMIT this->message(tr("Cannot rename '%1' to '%2': a sibling already has that name, or it "
                            "contains '/'.")
                           .arg(item->Name, name));
    return false;
  }
  // The rename triggers the Name property's event, and the deferred rebuild
  // refreshes the label.
  return true;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqSceneGraphModel::flags(const QModelIndex& index) const
{
  auto* item = pqInternals::item(index);
  if (!item)
  {
    return Qt::NoItemFlags;
  }
  Qt::ItemFlags result =
    Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
  if (item->Parent != &this->Internals->Top)
  {
    // The root cannot be dragged/reparented
    result |= Qt::ItemIsDragEnabled;
  }
  return result;
}

//-----------------------------------------------------------------------------
Qt::DropActions pqSceneGraphModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

//-----------------------------------------------------------------------------
QStringList pqSceneGraphModel::mimeTypes() const
{
  return QStringList{ NODE_MIME_TYPE };
}

//-----------------------------------------------------------------------------
QMimeData* pqSceneGraphModel::mimeData(const QModelIndexList& indexes) const
{
  // Dragging reparents at most one node, the payload is the node's global ID
  for (const QModelIndex& index : indexes)
  {
    if (auto* item = pqInternals::item(index))
    {
      if (item->Node)
      {
        auto* mime = new QMimeData();
        mime->setData(NODE_MIME_TYPE, QByteArray::number(item->Node->GetGlobalID()));
        return mime;
      }
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
bool pqSceneGraphModel::canDropMimeData(
  const QMimeData* data, Qt::DropAction action, int, int, const QModelIndex& parentIndex) const
{
  if (action != Qt::MoveAction || !data || !data->hasFormat(NODE_MIME_TYPE))
  {
    return false;
  }
  auto* target = pqInternals::item(parentIndex);
  if (!target)
  {
    // Nowhere to drop
    return false;
  }
  auto* source = this->Internals->draggedItem(data);
  if (!source)
  {
    return false;
  }
  return !pqInternals::isSameOrBelow(target, source);
}

//-----------------------------------------------------------------------------
bool pqSceneGraphModel::dropMimeData(
  const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parentIndex)
{
  if (!this->canDropMimeData(data, action, row, column, parentIndex))
  {
    return false;
  }
  auto* graph = this->Internals->Graph.GetPointer();
  auto* target = pqInternals::item(parentIndex);
  auto* source = this->Internals->draggedItem(data);
  if (!graph || !source || !source->Node || !target->Node)
  {
    return false;
  }
  if (!graph->MoveNode(source->Node, target->Node))
  {
    Q_EMIT this->message(tr("Cannot move '%1' under '%2'.").arg(source->Name, target->Name));
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void pqSceneGraphModel::onNodeProperty(vtkObject*, unsigned long, void* callData)
{
  const char* name = static_cast<const char*>(callData);
  if (name && (std::strcmp(name, "Children") == 0 || std::strcmp(name, "Name") == 0))
  {
    this->Internals->PendingRebuild = true;
  }
  else
  {
    this->Internals->PendingContent = true;
  }
  this->Internals->Timer.start();
}

//-----------------------------------------------------------------------------
void pqSceneGraphModel::onGraphProperty(vtkObject*, unsigned long, void* callData)
{
  const char* name = static_cast<const char*>(callData);
  if (name && std::strcmp(name, "Root") == 0)
  {
    this->Internals->PendingRebuild = true;
  }
  else
  {
    this->Internals->PendingContent = true;
  }
  this->Internals->Timer.start();
}

//-----------------------------------------------------------------------------
void pqSceneGraphModel::onRegistration(vtkObject*, unsigned long, void* callData)
{
  // Node are registered in a single session-wide group, so this also fires for
  // nodes of other graphs. Telling those apart would require a search of the
  // tree, which is essentially what a full rebuild costs, so just rebuild.
  auto* info = static_cast<vtkSMProxyManager::RegisteredProxyInformation*>(callData);
  if (!info || !(info->Type & vtkSMProxyManager::RegisteredProxyInformation::PROXY) ||
    !info->GroupName || std::strcmp(info->GroupName, vtkSMSceneGraphProxy::GetNodeGroup()) != 0)
  {
    return;
  }
  this->Internals->PendingRebuild = true;
  this->Internals->Timer.start();
}

//-----------------------------------------------------------------------------
void pqSceneGraphModel::flush()
{
  auto& internals = *this->Internals;
  const bool doRebuild = internals.PendingRebuild;
  const bool doContent = internals.PendingContent;
  internals.PendingRebuild = internals.PendingContent = false;
  if (doRebuild)
  {
    this->rebuild();
  }
  if (doContent)
  {
    Q_EMIT this->contentChanged();
  }
}

//-----------------------------------------------------------------------------
void pqSceneGraphModel::rebuild()
{
  auto& internals = *this->Internals;
  this->beginResetModel();
  internals.detachObservers();

  auto* graph = internals.Graph.GetPointer();
  if (graph)
  {
    // Rebuilding the mirrored structure requires one walk down from the root.
    // Similar to vtkSMSceneGraphProxy, guard against malformed graphs that were
    // externally produced.
    std::set<vtkSMSceneGraphNodeProxy*> seen;
    std::vector<std::pair<pqInternals::Item*, vtkSMSceneGraphNodeProxy*>> pending;
    pending.emplace_back(&internals.Top, graph->GetRoot());
    while (!pending.empty())
    {
      auto [parentItem, node] = pending.back();
      pending.pop_back();
      if (!node || !seen.insert(node).second)
      {
        continue;
      }
      auto item = std::make_unique<pqInternals::Item>();
      item->Parent = parentItem;
      item->Row = static_cast<int>(parentItem->Children.size());
      item->Node = node;
      item->Name = QString::fromUtf8(graph->GetNodeName(node));
      item->ObserverTag = node->AddObserver(
        vtkCommand::PropertyModifiedEvent, this, &pqSceneGraphModel::onNodeProperty);
      internals.Lookup[node] = item.get();
      auto* raw = item.get();
      parentItem->Children.push_back(std::move(item));

      vtkNew<vtkCollection> children;
      graph->GetChildren(node, children);
      // Push in reverse so they pop in order and keep their rows.
      for (int cc = children->GetNumberOfItems() - 1; cc >= 0; --cc)
      {
        pending.emplace_back(
          raw, vtkSMSceneGraphNodeProxy::SafeDownCast(children->GetItemAsObject(cc)));
      }
    }
  }

  this->endResetModel();
  Q_EMIT this->rebuilt();
}
