// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqSceneGraphModel
 * @brief   Qt item model mirroring a scenegraph
 *
 * Rather than repeatedly querying the vtkSMSceneGraphProxy, the model mirrors
 * the tree structure and nodes (one item per node, holding its parent, its
 * children and its name). The model is rebuilt from the graph whenever the
 * graph changes.
 *
 * The model observes vtkCommand::PropertyModifiedEvent on the node and graph
 * proxies, as well as node registration events on the proxy manager. To avoid
 * rebuilding intermediate states, each event starts a zero-delay timer, deferring
 * rebuild until control returns to the event loop.
 */
#ifndef pqSceneGraphModel_h
#define pqSceneGraphModel_h

#include <QAbstractItemModel>

#include <memory> // for unique_ptr

class vtkObject;
class vtkSMSceneGraphNodeProxy;
class vtkSMSceneGraphProxy;

class pqSceneGraphModel : public QAbstractItemModel
{
  Q_OBJECT
  typedef QAbstractItemModel Superclass;

public:
  pqSceneGraphModel(QObject* parent = nullptr);
  ~pqSceneGraphModel() override;

  /**
   * Show `graph`, or nothing when null. Rebuilds immediately.
   */
  void setGraph(vtkSMSceneGraphProxy* graph);
  vtkSMSceneGraphProxy* graph() const;

  /**
   * Returns the node referred to by `index`, or null for an invalid index or
   * a node that has since been destroyed.
   */
  vtkSMSceneGraphNodeProxy* node(const QModelIndex& index) const;

  /**
   * Returns the index showing `node`, or an invalid index if it is not in the
   * mirrored structure.
   */
  QModelIndex indexOf(vtkSMSceneGraphNodeProxy* node) const;

  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  Qt::DropActions supportedDropActions() const override;
  QStringList mimeTypes() const override;
  QMimeData* mimeData(const QModelIndexList& indexes) const override;
  bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
    const QModelIndex& parent) const override;
  bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
    const QModelIndex& parent) override;

Q_SIGNALS:
  /**
   * Emitted when the mirrored structure was rebuilt, because the tree's shape
   * or a node's name changed, or the graph was replaced.
   */
  void rebuilt();

  /**
   * Emitted on non-shape related changes, e.g. a node's transform or bindings,
   * or the graph's target views.
   */
  void contentChanged();

  /**
   * Emitted when a rename or drop was refused, contains a text reason.
   */
  void message(const QString& text);

private Q_SLOTS:
  void flush();

private:
  Q_DISABLE_COPY(pqSceneGraphModel)

  void onNodeProperty(vtkObject* caller, unsigned long eventId, void* callData);
  void onGraphProperty(vtkObject* caller, unsigned long eventId, void* callData);
  void onRegistration(vtkObject* caller, unsigned long eventId, void* callData);
  void rebuild();

  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
