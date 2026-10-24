"""Scaffolding checks

Tests that the plugin loads, the XML definitions resolve to our C++ classes, and
a graph can be created, found, enumerated, and given a root.

Run directly with pvpython; the plugin is found by name:

    bin/pvpython .../TestSceneGraphScaffold.py
"""


from paraview.simple import LoadDistributedPlugin
from paraview import servermanager as sm
from paraview.incubator import scenegraph as sg
from paraview.incubator.vtkPVIncubatorSceneGraph import (
    vtkSMSceneGraphProxy,
    vtkSMSceneGraphNodeProxy,
)


LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())
pxm = sm.ActiveConnection.Session.GetSessionProxyManager()


def check_definitions_resolve_to_our_classes():
    """The XML tag name selects the proxy class, so a typo in either the tag or
    the class name shows up as a plain vtkSMProxy rather than an error."""
    graph = pxm.NewProxy("scenegraph", "SceneGraph")
    assert vtkSMSceneGraphProxy.SafeDownCast(graph) is not None, (
        "<SceneGraphProxy> did not produce a vtkSMSceneGraphProxy"
    )
    graph.UnRegister(None)

    node = pxm.NewProxy("scenegraph", "SceneGraphNode")
    assert vtkSMSceneGraphNodeProxy.SafeDownCast(node) is not None, (
        "<SceneGraphNodeProxy> did not produce a vtkSMSceneGraphNodeProxy"
    )
    node.UnRegister(None)


def check_graph_lifecycle():
    graph = sg.create()

    handle = pxm.GetProxyName("scenegraphs", graph.SMProxy)
    assert handle, "the new graph was not registered in the scenegraphs group"
    return graph


def check_names_are_made_unique():
    """The proxy manager does not do this for us: a registration name maps to a
    *list* of proxies, so asking twice for one name succeeds and leaves the
    second graph permanently unreachable by name."""
    first = sg.create("Shared")
    second = sg.create("Shared")

    assert first != second, "two calls should give two graphs"
    names = {pxm.GetProxyName("scenegraphs", g.SMProxy) for g in (first, second)}
    assert len(names) == 2, "both graphs were registered as %r" % names
    assert sg.find("Shared") == first
    assert set(sg.all_graphs()) >= {first, second}


def check_deleting_a_graph_takes_its_nodes():
    graph = sg.create("Doomed")
    table = graph.CreateChild(graph.GetRoot(), "Table")
    graph.CreateChild(table, "Cup")

    before = pxm.GetNumberOfProxies("scenegraph_nodes")

    sg.delete(graph)

    # Root, Table and Cup go with it.
    after = pxm.GetNumberOfProxies("scenegraph_nodes")
    assert after == before - 3, "expected 3 nodes unregistered (%d -> %d)" % (before, after)


def check_root(graph):
    root = graph.GetRoot()
    assert root is not None, "GetRoot returned None"

    name = root.GetProperty("Name").GetElement(0)
    assert name == "World", "expected the root to be named World, got %r" % name

    # A second call must return the same node, not create another one.
    assert graph.GetRoot() is root, "GetRoot created a second root"

    count = pxm.GetNumberOfProxies("scenegraph_nodes")
    assert count == 1, "expected exactly 1 registered node, got %d" % count

    # The default local transform is the identity, and pushing it must not
    # trigger the client-server layer
    transform = root.GetProperty("LocalTransform")
    assert transform.GetNumberOfElements() == 16, "LocalTransform is not 16 elements"
    identity = [1 if i % 5 == 0 else 0 for i in range(16)]
    actual = [transform.GetElement(i) for i in range(16)]
    assert actual == identity, "expected an identity LocalTransform, got %r" % actual


check_definitions_resolve_to_our_classes()
graph = check_graph_lifecycle()
check_root(graph)
check_names_are_made_unique()
check_deleting_a_graph_takes_its_nodes()

print("TestSceneGraphScaffold: OK")
