# SceneGraph plugin

Adds the notion of a scenegraph to ParaView, in the form of a plugin and
Incubator module. Each named scenegraph is a tree of nodes that relates the
placement of objects in the pipeline.

Each node stores a local 4x4 transform and a list of child nodes. A node's
cumulative transform is its parent's world transform composed with its own, so
moving a node moves everything beneath it. Nodes are bound to pipeline
`(source, port)` pairs, and each scenegraph targets one or more 3D render
views. Applying the graph involves, for each binding, finding the associated
representation in each target view and writing the node's cumulative transform
to that representation's `UserTransform`.

Currently graph nodes only hold a transform, to control bound source position
and orientation, but the goal is for node eventually to support more (and possibly
arbitrary) properties that map to and control other Representation properties
besides just UserTransform.

The graph can be controlled by a UI dock panel or by a Python script.

## Layout

The wrappable core lives in `Incubator/SceneGraph/` so that it gets both
client-server as well as python wrapping, This plugin supplies the proxy XML,
the GUI, all the tests, and the packaging, and it depends on the incubator
module.

- `SceneGraph_SM.xml`: proxy definitions for `SceneGraph` and `SceneGraphNode`.
- `Incubator/SceneGraph/vtkSMSceneGraphProxy`: the "manager", contains all logic
- `Incubator/SceneGraph/vtkSMSceneGraphNodeProxy`: just a typed node handle

Neither proxy has a backing VTK object, and the graph is entirely client-side
server-manager state. The manager reads and writes it through `vtkSMPropertyHelper`,
and ParaView's generic proxy machinery saves and restores it, including the node
hierarchy, which is held in a repeatable `Children` proxy-property.

## Using the SceneGraph from the GUI

Loading the plugin adds the scene graph dock panel. You can select an existing
graph or create a new one at the top of the panel. Clicking New asks for a name,
then adds a graph with the given name. The Rename and Delete buttons act on
whatever graph is selected in the dropdown. Select a node and click Add Child
to begin building the graph, or rename a node by double-clicking it. A node can
be reparented by dragging it onto another node. Delete Node removes a node and
moves/expands its children into the empty slot. Delete Subtree removes a node
along with everything beneath it. The translation fields edit the selected
node's local transform and immediately apply it. Bind Active Source binds the
currently selected pipeline object (port included) to the currently selected
graph node. Target Active View adds the active render view to the selected
graph, so one graph can mirror a graph across several views. Edits made from
the Python shell show up in the panel.

## Using it from Python

The `SceneGraphProxy` class is automatically wrapped in Python, and most graph
API is directly available on that class. The `paraview.incubator.scenegraph`
module adds a small amount of convenience api not available in the proxy class,
such as returning lists and matrices instead of filling out-parameters.

```python
from paraview.simple import LoadDistributedPlugin, Sphere, Show
from paraview.incubator import scenegraph as sg

LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())

graph = sg.create()                                   # targets the active view
table = graph.CreateChild(graph.GetRoot(), "Table")
cup = graph.CreateChild(table, "Cup")

sphere = Sphere()
Show(sphere)
graph.AddSource(cup, sphere.SMProxy)

graph.SetLocalTransform(table, some_vtkMatrix4x4)     # stores only
graph.TraverseAndApply()                              # writes UserTransform

graph.GetChildren(table)                              # -> [node]
graph.GetWorldTransform(cup)                          # -> vtkMatrix4x4
```

To keep in the implementation as simple as possible, the api does not provide
any way to create a node in an unattached state. Nodes are created attached (via
`CreateChild`) and stay attached, but can be reparented via `MoveNode`. Sibling
order is considered irrelevant (other than for UI presentation), and is thus not
exposed in the API.

## Building

To build the SceneGraph plugin, enable it with:

```
-DPARAVIEW_PLUGIN_ENABLE_SceneGraph:BOOL=ON
```

## Tests

The plugin includes a number of tests, exercising both the Python api and the
GUI. If testing is enabled, tests can be run with:

```
ctest -R SceneGraph
```

Since the Python tests find the plugin by name, any of them can also be run
 directly:

```
bin/pvpython .../TestSceneGraphTree.py
```

`TestSceneGraphState.py` is the exception, as it requires extra parameters
(`save|load` and a statefile path).
