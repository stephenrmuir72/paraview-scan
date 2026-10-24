"""The SceneGraph wrapper

This module tests things the Python wrapper changes about calling the C++ from
Python, e.g. that a forwarded method raises SceneGraphError when the call recorded
an error and returns its result otherwise, the out-parameter methods return lists
and matrices, and the lifecycle functions return wrappers that compare by proxy.

Run directly with pvpython; the plugin is found by name:

    bin/pvpython .../TestSceneGraphWrapper.py
"""

from paraview.simple import CreateRenderView, LoadDistributedPlugin, SetActiveView, Show, Sphere
from paraview.incubator import scenegraph as sg
from paraview.incubator.vtkPVIncubatorSceneGraph import vtkSMSceneGraphProxy as SG
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from scenegraph_testing import refuses


LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())

# Create scene proxies once, since it's expensive
VIEW = CreateRenderView()
SPHERE = Sphere(registrationName="Bound")
Show(SPHERE, VIEW)


def scene():
    """Return a fresh graph targeting the shared view

    First removes the previous check's graphs."""
    for graph in sg.all_graphs():
        sg.delete(graph)
    SetActiveView(VIEW)
    graph = sg.create("Wrapped")
    root = graph.GetRoot()
    table = graph.CreateChild(root, "Table")
    cup = graph.CreateChild(table, "Cup")
    return graph, root, table, cup, SPHERE.SMProxy, VIEW.SMProxy


def stranger():
    """A node that belongs to some other graph."""
    other = sg.create("Other")
    return other.CreateChild(other.GetRoot(), "Stranger")


def check_lifecycle_hands_out_wrappers():
    graph = sg.create("Lifecycle")
    assert isinstance(graph, sg.SceneGraph)
    assert isinstance(graph.SMProxy, SG)
    assert "Lifecycle" in repr(graph), repr(graph)

    found = sg.find("Lifecycle")
    assert found is not graph and found == graph, "two wrappers of one proxy compare equal"
    assert graph == graph.SMProxy, "a wrapper compares equal to its own proxy"
    assert len({graph, found}) == 1, "and they hash alike"
    assert graph in sg.all_graphs()
    assert sg.find("NoSuchGraph") is None

    sg.rename(graph, "Renamed")
    assert sg.find("Renamed") == graph
    assert sg.find("Lifecycle") is None

    sg.delete(graph)
    assert sg.find("Renamed") is None
    assert graph not in sg.all_graphs()


def check_forwarded_calls_return_or_raise():
    graph, root, table, cup, sphere, view = scene()
    assert graph.GetParent(cup) is table
    assert graph.GetParent(root) is None, "the root's parent is a legitimate None"
    assert graph.FindNode("Nope") is None, "a missing node is a legitimate None"
    assert graph.RenameNode(cup, "Mug") is True, "a bool result comes back as it is"
    assert graph.GetErrorCode() == SG.NoError

    error = refuses(lambda: graph.GetParent(stranger()), SG.NotInGraph)
    assert isinstance(error, ValueError)
    assert error.name == "NotInGraph", error.name
    assert str(error) == graph.GetErrorMessage(), "the message is the C++ message"
    assert graph.GetErrorCode() == SG.NotInGraph, "the code is still readable after the raise"
    assert graph.GetProperty("Root") is not None, (
        "an inherited vtkSMProxy method is forwarded without reading the stale code"
    )


def check_out_parameters_become_return_values():
    graph, root, table, cup, sphere, view = scene()
    assert graph.GetChildren(table) == [cup]
    assert graph.GetChildren(cup) == []
    refuses(lambda: graph.GetChildren(None), SG.NullArgument)  # not an empty list
    assert graph.GetNodesByName("Cup") == [cup]
    assert graph.GetTargetViews() == [view]

    graph.AddSource(cup, sphere)
    representations = graph.GetDrivenRepresentations(cup, view)
    assert len(representations) == 1, representations
    assert representations[0].GetProperty("UserTransform") is not None

    local = graph.GetLocalTransform(cup)
    world = graph.GetWorldTransform(cup)
    assert isinstance(local, vtkMatrix4x4) and isinstance(world, vtkMatrix4x4)
    assert local.IsIdentity() and world.IsIdentity()

    foreign = stranger()
    refuses(lambda: graph.GetWorldTransform(foreign), SG.NotInGraph)
    assert graph.GetLocalTransform(foreign).IsIdentity(), "a reader accepts a foreign node"


check_lifecycle_hands_out_wrappers()
check_forwarded_calls_return_or_raise()
check_out_parameters_become_return_values()

print("TestSceneGraphWrapper: OK")
