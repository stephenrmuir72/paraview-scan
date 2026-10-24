// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMSceneGraphProxy
 * @brief   manager for a ParaView scenegraph
 *
 * A scenegraph relates the placement and other representational properties
 * of objects in ParaView's pipeline. The nodes of the graph form a tree, and
 * nodes hold property overrides to be applied to Represenation proxies.
 *
 * For example, each node holds a local 4x4 transform, and a node's "world
 * transform" is the composition of its ancestors' transforms with its own.
 * Nodes bind pipeline `(source, port)` pairs, and the graph as a whole targets
 * one or more 3D render views. Applying the graph resolves each binding to a
 * representation in each target view and writes the node's world matrix to
 * that representation's `UserTransform` property, so moving a parent node
 * moves its whole subtree.
 *
 * The proxy is client-side only, a driver that wants the graph applied (e.g.
 * the GUI, a Python script, or the CAVEInteraction plugin) must `SafeDownCast`
 * a registered `scenegraphs` proxy to this type and call `TraverseAndApply()`.
 *
 * SceneGraph instances are ordinary registered proxies, and there may be any
 * number of them in a session, each registered under a unique name in the
 * `scenegraphs` group. Use the provided static helpers to create, find,
 * enumerate, rename and delete them.
 *
 * Eight conventions are followed throughout the API:
 *
 * - **Ownership** Every node handed back is owned by the proxy manager, so a
 *   caller never holds a reference to release. Creating a node registers it,
 *   and that registration is what keeps it alive.
 * - **Every node is in the tree.** A node is created attached and stays
 *   attached until it is destroyed. `CreateChild()` is the only way to make
 *   a graph node, and `MoveNode()` the only way to reparent one.
 * - **Parentage is derived, rather than stored.** A node's parent is found by
 *   searching down from the root, so nothing can go stale, including after a
 *   statefile load. Every lookup is one walk of the graph, whether it's for
 *   a single parent or the whole chain of ancestors (which is gathered by
 *   one downward walk that notes each node's parent as it goes).
 * - **Sibling names are unique.** This is what makes a slash-separated path
 *   unambiguous. Attaching a node whose name is already taken renames it (a
 *   number is appended), while renaming a node to a taken name fails.
 * - **Nothing half-applies.** A rejected call leaves the graph exactly as it
 *   was. Methods that walk or change the tree refuse to do so when given a
 *   node from another graph. On the other hand, methods that only read a
 *   node's own state, such as its name, child count or local transform, do
 *   not check.
 * - **The error code describes the most recent call.** `GetErrorCode()` and
 *   `GetErrorMessage()` are reset at the start of every public method and set
 *   by every refusal, so they always describe the last call, including calls
 *   that return nothing. A walk that reaches a node twice sets `NotATree`,
 *   possibly beside an ordinary result. Refusals are treated as warnings,
 *   while errors are reserved for ParaView itself failing.
 * - **Child order is arbitrary.** New children are appended and every operation
 *   preserves the resulting order, but nothing depends on that order.
 * - **No guarantees on graphs built outside this API** The methods here
 *   keep the graph a tree with unique sibling names free of '/', but a direct
 *   edit of a graph's or a node's properties, or a hand written/edited
 *   statefile, can store anything. The methods that traverse are intended to
 *   be resilient to such graphs (allowing them to finish), since they refuse
 *   to visit a node twice, instead setting `NotATree`. Beyond that, however,
 *   results are undefined. A search stops at its target, and as such, is not
 *   guaranteed to encounter the problem in any given graph. Editing a graph
 *   that already has some kind of externally introduced problem may leave
 *   the graph in a worse state. `ValidateStructure()` is the complete check
 *   for the defects it knows about, and each graph runs it on itself after a
 *   statefile load.
 *
 * @sa vtkSMSceneGraphNodeProxy
 */
#ifndef vtkSMSceneGraphProxy_h
#define vtkSMSceneGraphProxy_h

#include "vtkPVIncubatorSceneGraphModule.h" // for export macro
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h" // for the watched proxy manager

#include <string> // for GetPath
#include <vector> // for CollectAncestors

class vtkCollection;
class vtkMatrix4x4;
class vtkSMSceneGraphNodeProxy;
class vtkSMSession;
class vtkSMSessionProxyManager;
class vtkSMSourceProxy;
class vtkSMViewProxy;

class VTKPVINCUBATORSCENEGRAPH_EXPORT vtkSMSceneGraphProxy : public vtkSMProxy
{
public:
  static vtkSMSceneGraphProxy* New();
  vtkTypeMacro(vtkSMSceneGraphProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The error code associated with the most recent call, possibly NoError.
   *
   * NullArgument:     a required argument was null or empty
   * NotInGraph:       the node is not reachable from this graph's root
   * NotATree:         a walk reached some node twice (see ValidateStructure())
   * NameTaken:        a sibling already has the name
   * NameHasSeparator: the name contains '/'
   * RootIsFixed:      the root cannot be moved or removed
   * WouldFormCycle:   the node would become its own ancestor
   * NotARenderView:   only 3D render views can be targeted
   * NotBound:         the source or view was not bound or targeted here
   * OutOfRange:       an index or port is out of range
   * NotRegistered:    the graph is not registered with the proxy manager
   * CreationFailed:   ParaView could not create a proxy
   * NotANode:         a child list holds something that is not a node
   * NotASource:       a binding refers to something that is not a pipeline source
   * BindingConflict:  another graph binds the same source and port in a view both target
   */
  enum ErrorCode
  {
    NoError = 0,
    NullArgument,
    NotInGraph,
    NotATree,
    NameTaken,
    NameHasSeparator,
    RootIsFixed,
    WouldFormCycle,
    NotARenderView,
    NotBound,
    OutOfRange,
    NotRegistered,
    CreationFailed,
    NotANode,
    NotASource,
    BindingConflict
  };

  /**
   * Return the error code recorded by the most recent public call. Any method
   * can set an error and might still return a value. Check error code after each
   * call to know whether the result is reliable.
   */
  ErrorCode GetErrorCode() { return this->LastError; }

  /**
   * Return the text associated with the most recent public call. Provides
   * any specific details regarding the code returned by GetErrorCode(). Could be
   * empty.
   */
  const char* GetErrorMessage() { return this->LastErrorMessage.c_str(); }

  /**
   * The fixed XML group the proxy definitions live in, as passed to
   * vtkSMSessionProxyManager::NewProxy(). Names proxy types, while the
   * registration groups below name live instances under arbitrary unique
   * handles.
   */
  static const char* GetDefinitionGroup() { return "scenegraph"; }

  /**
   * The registration group live graphs are registered in.
   */
  static const char* GetSceneGraphGroup() { return "scenegraphs"; }

  /**
   * The registration group live nodes are registered in.
   */
  static const char* GetNodeGroup() { return "scenegraph_nodes"; }

  /**
   * Create a graph, registered under `name`, or under an auto-generated unique
   * name if `name` is null. The returned graph already has its root in place
   * and targets the active render view.
   *
   * `name` is made unique first, since the proxy manager does not do it.
   */
  static vtkSMSceneGraphProxy* CreateSceneGraph(
    vtkSMSessionProxyManager* pxm, const char* name = nullptr);

  /**
   * Return the graph registered under `name`, or null if there is none.
   */
  static vtkSMSceneGraphProxy* FindSceneGraph(vtkSMSessionProxyManager* pxm, const char* name);

  /**
   * Collect and return every registered graph into `graphs`, replacing its contents.
   */
  static void GetSceneGraphs(vtkSMSessionProxyManager* pxm, vtkCollection* graphs);

  /**
   * Unregister `graph` along with every node in it, returning the properties of
   * any source (represenation) it was controlling to default values first.
   */
  static bool DeleteSceneGraph(vtkSMSessionProxyManager* pxm, vtkSMSceneGraphProxy* graph);

  /**
   * Re-register `graph` under `name`, which is first made unique the way
   * CreateSceneGraph() makes it. Registers the new name before dropping the old
   * one, since the registration is what holds the graph. Observers of the proxy
   * manager's `RegisterEvent` should be aware that a rename results in two such
   * events, and the graph appears to have both names after the first one.
   */
  static bool RenameSceneGraph(
    vtkSMSessionProxyManager* pxm, vtkSMSceneGraphProxy* graph, const char* name);

  /**
   * Return the always-present "World" root, created on first access and never
   * deleted.
   */
  vtkSMSceneGraphNodeProxy* GetRoot();

  /**
   * Return the display name of `node`.
   */
  const char* GetNodeName(vtkSMSceneGraphNodeProxy* node);

  /**
   * Return the number of direct children of the given `node`.
   */
  unsigned int GetNumberOfChildren(vtkSMSceneGraphNodeProxy* node);

  /**
   * Get the child of `node` at `index`, or null when `index` is out of range.
   */
  vtkSMSceneGraphNodeProxy* GetChild(vtkSMSceneGraphNodeProxy* node, unsigned int index);

  /**
   * Collect the children of `node`, in order, into `children`, replacing its
   * contents.
   */
  void GetChildren(vtkSMSceneGraphNodeProxy* node, vtkCollection* children);

  /**
   * Get the parent of `node`, or null for the root, which has no parent. Found
   * by searching down from the root rather than stored, so it cannot go stale.
   */
  vtkSMSceneGraphNodeProxy* GetParent(vtkSMSceneGraphNodeProxy* node);

  /**
   * Return `true` if the `node` lies in this graph anywhere below `ancestor`. A node
   * is not a descendant of itself.
   */
  bool IsDescendantOf(vtkSMSceneGraphNodeProxy* node, vtkSMSceneGraphNodeProxy* ancestor);

  /**
   * Get the number of edges from `node` up to the root (0 for the root itself).
   */
  int GetDepth(vtkSMSceneGraphNodeProxy* node);

  /**
   * Get the child of `parent` called `name` (searches the given `parent` only).
   */
  vtkSMSceneGraphNodeProxy* FindChild(vtkSMSceneGraphNodeProxy* parent, const char* name);

  /**
   * Get the node at a slash-separated `path` that begins with the root's name,
   * as GetPath() returns it, e.g. "World/Table/Cup". Returns null if no node
   * is at that path.
   */
  vtkSMSceneGraphNodeProxy* FindNode(const char* path);

  /**
   * Collect every node called `name`, anywhere in the graph, into `nodes`,
   * replacing its contents. Several nodes can match, since names are only
   * required to be unique among siblings.
   */
  void GetNodesByName(const char* name, vtkCollection* nodes);

  /**
   * Get the full path to `node`, beginning with the root's name.
   */
  std::string GetPath(vtkSMSceneGraphNodeProxy* node);

  /**
   * Create a node and append it to 'parent`. If the name is not unique among
   * siblings, it is made unique by adding a 1 (or a subsequent digit that is
   * not yet taken).
   *
   * Returns null if `parent` is not in this graph or the node cannot be made.
   */
  vtkSMSceneGraphNodeProxy* CreateChild(vtkSMSceneGraphNodeProxy* parent, const char* name);

  /**
   * Reparent `node` under `newParent`, appending it there. Refuses to move the
   * root, to move a node beneath itself or one of its own descendants, or to
   * move one into a `newParent` belonging to a different graph. If the name
   * of `node` collides with an existing child name, it is given a new name
   * which is unique among existing siblings.
   *
   * Moving a node to the parent it already has does nothing and succeeds.
   */
  bool MoveNode(vtkSMSceneGraphNodeProxy* node, vtkSMSceneGraphNodeProxy* newParent);

  /**
   * Rename `node`, failing if a sibling already uses `name`.
   *
   * Also fails for a `name` containing a '/', which would make GetPath() build
   * a path FindNode() cannot resolve.
   *
   * Fails for a node belonging to a different graph.
   */
  bool RenameNode(vtkSMSceneGraphNodeProxy* node, const char* name);

  /**
   * Remove `node`, splicing its children into its parent at the position it
   * occupied. The spliced children may be given new names if existing names
   * collide with those of the pre-existing children. The root node of a
   * scenegraph cannot be deleted.
   *
   * The children's own local transforms are left alone, so they visibly
   * move as the deleted node's transform drops out of their chain.
   *
   * Any sources bound to `node` return to default positions.
   */
  bool DeleteNode(vtkSMSceneGraphNodeProxy* node);

  /**
   * Remove `node` along with every one of its descendants, returning properties
   * of any bounds sources back to default values. The root cannot be removed.
   */
  bool RemoveSubtree(vtkSMSceneGraphNodeProxy* node);

  /**
   * Set the transform of the given node relative to its parent. This does
   * not change any Representation properties, but just stores the matrix
   * on the node.
   *
   * This method checks the node for membership in the graph, so call it at
   * most once per rendered frame.
   *
   * Fails for a node belonging to a different graph.
   */
  bool SetLocalTransform(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* matrix);

  /**
   * Copy the transform of `node` relative to its parent into `matrix`.
   */
  bool GetLocalTransform(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* matrix);

  /**
   * Copy the cumulative transform of `node` relative to the world into `matrix`,
   * that is its own local transform composed with every ancestor's.
   *
   * Computed on demand by walking down from the root, so it is correct the moment
   * a transform or the tree changes, with no traversal needed in between.
   */
  bool GetWorldTransform(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* matrix);

  /**
   * Bind one output port of a pipeline source to `node`, so that applying the
   * graph updates the Representation properties with overrides stored in the
   * node (possibly computed from parents).
   *
   * A given (source, port) pair is bound by at most one node in this graph.
   * Within this graph, binding a pair that another node already holds detaches
   * it from that node first, since two nodes writing one representation's
   * properties would fight over them. However, across graphs such a binding is
   * refused, i.e. if another graph binds it in a view both graphs target,
   * this call fails with BindingConflict, changing nothing. Graphs targeting
   * different views may bind the same (source, port) pair.
   *
   * Fails for a node belonging to a different graph.
   */
  bool AddSource(vtkSMSceneGraphNodeProxy* node, vtkSMSourceProxy* source, int port = 0);

  /**
   * Unbind one output port of a source from `node`, returning properties of
   * the target representation to its default values.
   */
  bool RemoveSource(vtkSMSceneGraphNodeProxy* node, vtkSMSourceProxy* source, int port = 0);

  /**
   * Get the number of bound sources associated with the given `node`.
   */
  unsigned int GetNumberOfSources(vtkSMSceneGraphNodeProxy* node);

  /**
   * Get a `node`'s bound source at `index`, or null when out of range.
   */
  vtkSMSourceProxy* GetSource(vtkSMSceneGraphNodeProxy* node, unsigned int index);

  /**
   * Get the output port of `node`'s bound source at `index`, or -1 when out of range.
   */
  int GetSourcePort(vtkSMSceneGraphNodeProxy* node, unsigned int index);

  /**
   * Get the node in this graph that binds `(source, port)`, or null if none does.
   */
  vtkSMSceneGraphNodeProxy* GetOwningNode(vtkSMSourceProxy* source, int port = 0);

  /**
   * Collect the representations `node`'s bindings actually resolve to in
   * `view`, replacing the contents of `reps`. Source that are not shown are not
   * included.
   */
  void GetDrivenRepresentations(
    vtkSMSceneGraphNodeProxy* node, vtkSMViewProxy* view, vtkCollection* reps);

  /**
   * Add a view for this graph to control. Only 3D render views are accepted.
   *
   * Refused with BindingConflict when another graph targeting the given view
   * already binds a source this graph binds, since both graphs would then fight
   * over that representation's properties.
   *
   * Targeting several views mirrors this graph across all of them. To let
   * views move independently, give each its own graph instead.
   */
  bool AddTargetView(vtkSMViewProxy* view);

  /**
   * Remove `view` from this graphs target views, returning properties of all
   * shown representations to default values.
   */
  bool RemoveTargetView(vtkSMViewProxy* view);

  /**
   * Collect this graph's target views into `views`, replacing its contents.
   */
  void GetTargetViews(vtkCollection* views);

  /**
   * Overridden to add observers of session proxy manager events. Observes
   * unregister events, so that a source removed from the pipeline does not
   * leave the graph holding a binding. Observes statefile load events in
   * order to validate the structure of scenegraphs loaded from state.
   */
  void SetSession(vtkSMSession* session) override;

  /**
   * Check the graph for structural damage, returning true when it is sound and
   * describing anything wrong as warnings.
   *
   * This method checks the following: the root node listed as a child, one
   * node listed as a child of two parents, entries in a child list that are
   * not nodes, bindings to something that is not a pipeline source, bindings
   * that another graph also binds in a view both graphs target, siblings that
   * share a name, and names containing a '/'.
   *
   * This method is invoked after a statefile load to detect the flaws
   * mentioned above, and defects are only reported, no attempt is made to
   * fix them.
   */
  bool ValidateStructure();

  /**
   * Walk the tree top-down, composing or overriding each node's properties, and
   * writing the result to the representations its bound sources resolve to in
   * each target view.
   *
   * Bindings are resolved afresh every time, so showing or hiding a source is
   * picked up on the next call with no bookkeeping. Writes are compared against
   * what each representation already has set and not executed unless the value
   * is different.
   */
  void TraverseAndApply();

protected:
  vtkSMSceneGraphProxy();
  ~vtkSMSceneGraphProxy() override;

  /**
   * Return true if the given `node` is part of this graph. Returns false
   * without an error for a node that is not, and fails with NotATree if the
   * search found that the graph is not a tree.
   */
  bool Contains(vtkSMSceneGraphNodeProxy* node);

  /**
   * Like Contains(), but also fails with NotInGraph for a node that is not
   * part of this graph.
   */
  bool RequireContains(vtkSMSceneGraphNodeProxy* node);

  /**
   * Return `name` with any path separators replaced by underscores. Passing
   * an empty string or nullptr returns the default node name, "Node".
   */
  static std::string SanitizeNodeName(const char* name);

  /**
   * Create a node with no parent.
   *
   * Note: Callers outside this class should never be able to obtain a node
   * without a parent. This invariant is required for correctness throughout
   * this class.
   */
  vtkSMSceneGraphNodeProxy* CreateNode(const char* name);

  /**
   * Write `world` to the representations this node's bound sources resolve to
   * in each target view. Called for every node as the traversal reaches it.
   */
  void ApplyToNode(vtkSMSceneGraphNodeProxy* node, vtkMatrix4x4* world);

  /**
   * Put whatever `(source, port)` draws back at the identity transform, in
   * every target view. Used wherever the graph stops driving something, since
   * leaving an object displaced with nothing controlling it would give the
   * user no way to put it back.
   */
  void ResetBinding(vtkSMSourceProxy* source, int port);

  /**
   * ResetBinding() for every source bound to `node`.
   */
  void ResetBindingsOf(vtkSMSceneGraphNodeProxy* node);

  /**
   * Drop every binding to `source`, whatever port, from every node.
   *
   * Called when a source leaves the pipeline. ParaView allows that even while
   * we hold a reference to it, and without this the node would keep a binding
   * to something the user has deleted.
   */
  void UnbindSource(vtkSMSourceProxy* source);

  /**
   * Unregister a node and everything below it, resetting the properties of any
   * bound sources in the subtree first. Used by the removal methods after the
   * subtree has been detached from its parent.
   */
  void UnregisterSubtree(vtkSMSceneGraphNodeProxy* node);

  /**
   * Return `name` if no child of `parent` already uses it, otherwise return `name`
   * with the lowest number appended that makes it unique. `exclude` is a node
   * whose name should be ignored during the check, so that re-attaching a node
   * under the same parent does not collide with itself.
   */
  std::string MakeUniqueChildName(
    vtkSMSceneGraphNodeProxy* parent, const char* name, vtkSMSceneGraphNodeProxy* exclude);

  /**
   * Collects a node and its ancestors into the provided chain, ordered nearest
   * ancestor first. If the node is null or not in this graph, returns false and
   * sets the error code.
   */
  bool CollectAncestors(
    vtkSMSceneGraphNodeProxy* node, std::vector<vtkSMSceneGraphNodeProxy*>& chain);

  /**
   * Return another registered graph that targets the given view and binds
   * `(source, port)`, or null, if no such graph exists. The given node receives
   * the node holding the binding.
   */
  vtkSMSceneGraphProxy* FindOtherBinding(
    vtkSMSourceProxy* source, int port, vtkSMViewProxy* view, vtkSMSceneGraphNodeProxy*& node);

  /**
   * Sets the error code and message and emits message (as an error if internalFailure
   * is true, as a warning otherwise).
   */
  void Fail(ErrorCode code, const std::string& message, bool internalFailure = false);

  /**
   * Reset the error code and message. Every public method starts with this.
   */
  void ClearError();

  /**
   * Observer for the proxy manager's unregister event. Unbinds a source that
   * has left the `sources` group.
   */
  void OnProxyUnRegistered(vtkObject* caller, unsigned long eventId, void* callData);

  /**
   * Observer for the proxy manager's load-state event, which fires once the
   * whole file is loaded and registered.
   */
  void OnStateLoaded(vtkObject* caller, unsigned long eventId, void* callData);

private:
  vtkSMSceneGraphProxy(const vtkSMSceneGraphProxy&) = delete;
  void operator=(const vtkSMSceneGraphProxy&) = delete;

  void StopWatchingProxyManager();

  vtkWeakPointer<vtkSMSessionProxyManager> WatchedProxyManager;
  unsigned long UnRegisterObserverTag = 0;
  unsigned long LoadStateObserverTag = 0;

  ErrorCode LastError = NoError;
  std::string LastErrorMessage;
};

#endif
