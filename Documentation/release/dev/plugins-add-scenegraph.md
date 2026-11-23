## New SceneGraph plugin

ParaView has a new SceneGraph plugin for positioning pipeline objects relative to each other. A scenegraph is a named tree of nodes, and ParaView allows creating and managing any number of them. Each node has a local transform, and moving a node moves everything beneath it. You bind pipeline sources (and their output ports) to nodes and choose which render views the graph targets. Applying the graph sets each bound source's representation transform in those views from its node's position in the tree. Scenegraphs are saved to and loaded from state files.

Loading the plugin adds the _Scene Graph_ dock panel, where you can create, rename and delete graphs, add, rename, delete and reparent nodes (by dragging them onto other nodes), edit a node's translation, bind the active source to a node, and target the active view. Changes made from the _Python Shell_ show up in the panel.

From Python, the `paraview.incubator.scenegraph` module provides functions to create, find, rename and delete graphs, returns the graph objects you call the tree operations on, and raises `SceneGraphError` when an operation fails.

When the SceneGraph plugin is enabled at build time, the CAVEInteraction plugin applies every scenegraph that targets the active render view once per pass through its event loop, after updating its interactor styles and before rendering. The SceneGraph plugin's `Examples/Interactors/SceneGraphGrabNode.py` is an example CAVEInteraction interactor style that moves scenegraph nodes with a tracked controller.

The plugin is not built by default. Enable it with `PARAVIEW_PLUGIN_ENABLE_SceneGraph`.

### Developer notes

The scenegraph logic lives in the new `ParaViewIncubator::SceneGraph` incubator module, which is client-server and Python wrapped. `vtkSMSceneGraphProxy` implements the graph and its operations, and `vtkSMSceneGraphNodeProxy` is a typed handle for a node. Both are server-manager proxies defined by the plugin, so the plugin must be loaded for them to be created.
