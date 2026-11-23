r"""The ParaView scenegraph from Python.

This module provides a more ergonomic wrapper around the Python-wrapped
vtkSMSceneGraphProxy class. Similar to the Proxy objects returned by
paraview.simple, this class holds the proxy as SMProxy, and forwards method
calls to the wrapped object. It also checks the error code after each call to
raise exceptions when warranted.

Example usage:

    from paraview.incubator import scenegraph as sg

    graph = sg.create()                                  # or sg.find(name)
    table = graph.CreateChild(graph.GetRoot(), "Table")  # the C++ name
    graph.AddSource(table, sphere.SMProxy)               # idiomatic ParaView
    graph.TraverseAndApply()

    graph.GetChildren(table)                             # -> [node]
    graph.GetWorldTransform(table)                       # -> vtkMatrix4x4

What this wrapper adds to vtkSMSceneGraphProxy:

- Raises exceptions for error conditions: A C++ return value cannot always
  tell a refusal from an ordinary answer, e.g. the GetParent() method returns
  null for the root node as well as for a node from another graph. In order
  to properly communicate all possible states, the C++ records a code and a
  message on the graph after each call, accessible via GetErrorCode() and
  GetErrorMessage(). For convenience, the methods here read the code after
  each call, and if it is set, raise SceneGraphError with the error message.
- Returns lists and matrices: The C++ fills a vtkCollection or a
  vtkMatrix4x4 supplied by the caller, mainly so it's clear who is responsible
  for deleting memory. SceneGraph methods handle creating those objects for
  C++ to populate, and then returning them, as a convenience to callers. Note
  that returned lists (GetChildren, GetNodesByName, GetTargetViews,
  GetDrivenRepresentations) hold pointers to the proxies held on the C++
  side, not copies.
- Provides graph lifecycle methods via module-level functions: create, find,
  all_graphs, delete and rename wrap their static C++ counterparts,
  internally providing the session proxy manager users would otherwise have
  to fetch themselves.

Otherwise, SceneGraph method names match those on the C++ side. There is no
class provided to wrap vtkSMSceneGraphNodeProxy. Instead, a node is the
opaque handle the C++ returns, and every operation on it goes through the
graph. There is no notion of an active scenegraph, instead graphs are
created/found by name. Calls that need the bare proxy can access it with
via the SMProxy attribute.
"""

import inspect

from paraview import servermanager
from paraview.incubator.vtkPVIncubatorSceneGraph import vtkSMSceneGraphProxy
from paraview.modules.vtkRemotingServerManager import vtkSMProxy
from vtkmodules.vtkCommonCore import vtkCollection
from vtkmodules.vtkCommonMath import vtkMatrix4x4


# Map the wrapped error code enum values to their names
_CODE_NAMES = {
    int(value): name
    for name in dir(vtkSMSceneGraphProxy)
    for value in [getattr(vtkSMSceneGraphProxy, name)]
    if isinstance(value, vtkSMSceneGraphProxy.ErrorCode)
}

# Instance methods in vtkSMSceneGraphProxy reset the error code on entry and
# set it appropriately when errors are encountered. Static methods on
# vtkSMSceneGraphProxy and methods inherited from vtkSMProxy don't do
# that, so here we list those exemptions that should be forwarded without
# reading the error code.
_NOT_CHECKED = {
    "CreateSceneGraph", "FindSceneGraph", "GetSceneGraphs", "DeleteSceneGraph",
    "RenameSceneGraph", "GetDefinitionGroup", "GetSceneGraphGroup", "GetNodeGroup",
    "GetErrorCode", "GetErrorMessage",
} | set(vars(vtkSMProxy))

# The remaining methods in vtkSMSceneGraphProxy are not exempted, and we
# will check the error code after forwarding
_CHECKED = {
    name
    for name, value in vars(vtkSMSceneGraphProxy).items()
    if inspect.ismethoddescriptor(value)
    and not name.startswith("__")
    and name not in _NOT_CHECKED
}


class SceneGraphError(ValueError):
    """Raised when a scenegraph call recorded an error"""

    def __init__(self, code, message):
        """Construct a SceneGraphError given a code and error message"""
        super().__init__(message)
        self.code = code
        self.name = _CODE_NAMES.get(int(code), str(int(code)))


def _proxy_manager():
    """Return the active session's proxy manager"""
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    return servermanager.ActiveConnection.Session.GetSessionProxyManager()


def _as_list(collection):
    """Return the contents of a vtkCollection as a list"""
    return [
        collection.GetItemAsObject(i) for i in range(collection.GetNumberOfItems())
    ]


def _raise_on_error(proxy):
    code = proxy.GetErrorCode()
    if code != vtkSMSceneGraphProxy.NoError:
        raise SceneGraphError(code, proxy.GetErrorMessage())


def _bare(graph):
    """Return the vtkSMSceneGraphProxy associated with the given graph

    If the passed object does not have an SMProxy property, it is assumed
    to be vtkSMSceneGraphProxy itself."""
    return getattr(graph, "SMProxy", graph)


class SceneGraph:
    def __init__(self, proxy):
        self.SMProxy = proxy

    def __getattr__(self, name):
        if name == "SMProxy":
            # Prevent infinite recursion if the instance has no "SMProxy" yet
            raise AttributeError(name)

        attribute = getattr(self.SMProxy, name)

        if name not in _CHECKED:
            # Forward unchecked methods directly
            return attribute

        def forwarded(*args):
            result = attribute(*args)
            _raise_on_error(self.SMProxy)
            return result

        forwarded.__name__ = name
        forwarded.__doc__ = attribute.__doc__
        return forwarded

    def __eq__(self, other):
        """Two scenegraph objects are equal when they hold the same proxy"""
        return self.SMProxy == _bare(other)

    def __hash__(self):
        return hash(self.SMProxy)

    def __repr__(self):
        pxm = self.SMProxy.GetSessionProxyManager()
        if pxm is None:
            # Expected if Disconnect() was called while a script was holding this
            # scenegraph proxy (same for any proxy)
            return "SceneGraph(<no session>)"
        name = pxm.GetProxyName(vtkSMSceneGraphProxy.GetSceneGraphGroup(), self.SMProxy)
        return "SceneGraph(%r)" % name

    def GetErrorCode(self):
        """Return the error code set by the most recent call (does not raise)"""
        return self.SMProxy.GetErrorCode()

    def GetErrorMessage(self):
        """Return the message set by the most recent call (does not raise)"""
        return self.SMProxy.GetErrorMessage()

    def GetChildren(self, node):
        """Return a list containing the children of the given node"""
        found = vtkCollection()
        self.SMProxy.GetChildren(node, found)
        _raise_on_error(self.SMProxy)
        return _as_list(found)

    def GetNodesByName(self, name):
        """Return a list containing all nodes with the given name

        Since only siblings are required to have unique names, this can return
        multiple nodes."""
        found = vtkCollection()
        self.SMProxy.GetNodesByName(name, found)
        _raise_on_error(self.SMProxy)
        return _as_list(found)

    def GetTargetViews(self):
        """Return a list containing the views this graph targets"""
        found = vtkCollection()
        self.SMProxy.GetTargetViews(found)
        _raise_on_error(self.SMProxy)
        return _as_list(found)

    def GetDrivenRepresentations(self, node, view):
        """Return a list containing the representations affected by the given node

        The affected representations are those the node's bound sources resolve
        to in the given view. Sources that are not shown in that view do not
        result in a representation in the returned list."""
        found = vtkCollection()
        self.SMProxy.GetDrivenRepresentations(node, view, found)
        _raise_on_error(self.SMProxy)
        return _as_list(found)

    def GetLocalTransform(self, node):
        """Return the transform of the given node, relative to its parent

        Returns a new vtkMatrix4x4."""
        matrix = vtkMatrix4x4()
        self.SMProxy.GetLocalTransform(node, matrix)
        _raise_on_error(self.SMProxy)
        return matrix

    def GetWorldTransform(self, node):
        """Return the transform of the given node, relative to the world

        Returns a new vtkMatrix4x4. The returned transform is the node's local
        transform composed with those of it's ancestors in the graph."""
        matrix = vtkMatrix4x4()
        self.SMProxy.GetWorldTransform(node, matrix)
        _raise_on_error(self.SMProxy)
        return matrix


def create(name=None):
    """Create a scenegraph with the given name

    The graph is created with it's root node (named "World") already in
    place, and the graph targets the active render view. Raises if the proxy
    definition is not available, which means the SceneGraph plugin has not
    been loaded. If no name is provided, the graph will be created with a
    generated name.
    """
    proxy = vtkSMSceneGraphProxy.CreateSceneGraph(_proxy_manager(), name)
    if proxy is None:
        raise RuntimeError(
            "Could not create a scenegraph. Is the SceneGraph plugin loaded?"
        )
    return SceneGraph(proxy)


def find(name):
    """Return the scenegraph registered under the given name

    If there is no graph with the given name, return None."""
    proxy = vtkSMSceneGraphProxy.FindSceneGraph(_proxy_manager(), name)
    return None if proxy is None else SceneGraph(proxy)


def all_graphs():
    """Return a list containing all registered scenegraphs"""
    found = vtkCollection()
    vtkSMSceneGraphProxy.GetSceneGraphs(_proxy_manager(), found)
    return [SceneGraph(proxy) for proxy in _as_list(found)]


def delete(graph):
    """Unregister the given graph and destroy its nodes

    Resets the properties of any affected Representations (via bound source
    and targeted View) they back to their default values."""
    proxy = _bare(graph)
    vtkSMSceneGraphProxy.DeleteSceneGraph(_proxy_manager(), proxy)
    _raise_on_error(proxy)


def rename(graph, name):
    """Rename the given graph to the provided name

    Chooses a similar but unique name if another graph already has the
    given name."""
    proxy = _bare(graph)
    vtkSMSceneGraphProxy.RenameSceneGraph(_proxy_manager(), proxy, name)
    _raise_on_error(proxy)
