"""Statefile round-trip for a deep tree.

A second graph in the same file has a defect that only a direct property
edit can make, to show that each graph validates itself once the load is
complete.

Runs in two modes/phases, save and load:

    bin/pvpython .../TestSceneGraphState.py save /tmp/sg.pvsm
    bin/pvpython .../TestSceneGraphState.py load /tmp/sg.pvsm
"""

import sys

from paraview.simple import (
    CreateRenderView,
    LoadDistributedPlugin,
    LoadState,
    SaveState,
    Show,
    Sphere,
)
from paraview import servermanager as sm
from paraview.incubator import scenegraph as sg
from paraview.incubator.vtkPVIncubatorSceneGraph import vtkSMSceneGraphProxy as SG
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from scenegraph_testing import refuses


if len(sys.argv) < 3:
    raise SystemExit("usage: TestSceneGraphState.py <save|load> <statefile>")

mode, statefile = sys.argv[1], sys.argv[2]
LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())
pxm = sm.ActiveConnection.Session.GetSessionProxyManager()

# A transform we can easily recognize, translation (1, 2, 3)
TABLE_TRANSFORM = [
    1.0, 0.0, 0.0, 1.0,
    0.0, 1.0, 0.0, 2.0,
    0.0, 0.0, 1.0, 3.0,
    0.0, 0.0, 0.0, 1.0,
]


def child_names(graph, node):
    return [graph.GetNodeName(child) for child in graph.GetChildren(node)]


def table_transform():
    matrix = vtkMatrix4x4()
    matrix.DeepCopy(TABLE_TRANSFORM)
    return matrix


def elements(matrix):
    return [matrix.GetElement(i // 4, i % 4) for i in range(16)]


def save():
    graph = sg.create("TestGraph")
    root = graph.GetRoot()

    # Create a graph with at least one branch three levels below the root
    table = graph.CreateChild(root, "Table")
    graph.CreateChild(table, "Cup")
    plate = graph.CreateChild(table, "Plate")
    graph.CreateChild(plate, "Rim")
    graph.CreateChild(root, "Lamp")

    graph.SetLocalTransform(table, table_transform())

    # Bind a source and a target view
    view = CreateRenderView()
    sphere = Sphere(registrationName="BoundSphere")
    Show(sphere, view)
    graph.AddTargetView(view.SMProxy)
    graph.AddSource(table, sphere.SMProxy, 0)

    # Create a second graph with two siblings named alike. Since RenameNode does
    # not allow that, we have to write straight to the property.
    broken = sg.create("Broken")
    broken.CreateChild(broken.GetRoot(), "Cup")
    mug = broken.CreateChild(broken.GetRoot(), "Mug")
    mug.GetProperty("Name").SetElement(0, "Cup")
    mug.UpdateVTKObjects()
    refuses(lambda: broken.ValidateStructure(), SG.NameTaken)

    SaveState(statefile)
    print("saved %d nodes to %s" % (pxm.GetNumberOfProxies("scenegraph_nodes"), statefile))


def load():
    LoadState(statefile)

    graphs = sg.all_graphs()
    assert len(graphs) == 2, (
        "expected exactly 2 scenegraphs after load, got %d" % len(graphs)
    )
    graph = sg.find("TestGraph")
    assert graph is not None, "the graph came back under a different name"

    # Each graph validated itself when the load completed, and nothing has
    # called a method on either since, so each graph's error code should
    # still reflect the validation result.
    broken = sg.find("Broken")
    assert broken is not None, "the second graph did not come back"
    assert broken.GetErrorCode() == SG.NameTaken, (
        "the load-time check should have reported the duplicate names, code %d: %r"
        % (broken.GetErrorCode(), broken.GetErrorMessage())
    )
    assert broken.GetErrorMessage(), "a reported defect should carry a message"
    assert graph.GetErrorCode() == SG.NoError, (
        "the sound graph should have passed the load-time check: %r" % graph.GetErrorMessage()
    )

    # If Root did not come through the state load safely, GetRoot() will
    # quietly manufacture a fresh empty "World". We'll count its children
    # to be sure that didn't happen.
    root = graph.GetRoot()
    assert root is not None, "no root after load"
    assert graph.GetNodeName(root) == "World", (
        "root is named %r" % graph.GetNodeName(root)
    )

    # Now there should be six nodes in TestGraph, three in Broken.
    node_count = pxm.GetNumberOfProxies("scenegraph_nodes")
    assert node_count == 9, "expected 9 nodes after load, got %d" % node_count

    assert child_names(graph, root) == ["Table", "Lamp"], child_names(graph, root)

    table = graph.FindNode("World/Table")
    assert table is not None, "World/Table did not survive the round trip"
    assert child_names(graph, table) == ["Cup", "Plate"], child_names(graph, table)

    rim = graph.FindNode("World/Table/Plate/Rim")
    assert rim is not None, "the third level did not survive the round trip"
    assert graph.GetPath(rim) == "World/Table/Plate/Rim"
    assert graph.GetDepth(rim) == 3, "expected depth 3, got %d" % graph.GetDepth(rim)

    # Parenting is reconstructed from the restored child lists.
    assert graph.GetParent(rim) is graph.FindNode("World/Table/Plate"), (
        "parent lookup is wrong after load"
    )

    actual = elements(graph.GetLocalTransform(table))
    assert actual == TABLE_TRANSFORM, "LocalTransform did not round trip: %r" % actual

    # The source binding survived, pointing at the reloaded source rather than
    # a stale id.
    assert graph.GetNumberOfSources(table) == 1, (
        "expected Table to still have 1 bound source, got %d"
        % graph.GetNumberOfSources(table)
    )
    source = graph.GetSource(table, 0)
    assert source is not None, "the bound source did not round trip"
    assert source.GetXMLName() == "SphereSource", source.GetXMLName()
    assert graph.GetSourcePort(table, 0) == 0
    assert graph.GetOwningNode(source, 0) is table, (
        "the reverse lookup should still find Table"
    )

    # Verify the target view was properly reconstituted as well
    views = graph.GetTargetViews()
    assert len(views) == 1, "expected 1 target view after load, got %d" % len(views)

    # Now check the whole thing is still structurally sound
    assert graph.ValidateStructure(), "the reloaded graph failed validation"

    # Ensure the reloaded graph still is still controlling its representation
    graph.TraverseAndApply()
    reps = graph.GetDrivenRepresentations(table, views[0])
    assert len(reps) == 1, (
        "the reloaded binding should resolve to a representation, got %d" % len(reps)
    )
    ut = [reps[0].GetProperty("UserTransform").GetElement(i) for i in range(16)]
    assert ut == TABLE_TRANSFORM, (
        "applying the reloaded graph should transform the representation: %r" % ut
    )

    print("TestSceneGraphState: OK")


if mode == "save":
    save()
elif mode == "load":
    load()
else:
    raise SystemExit("mode must be 'save' or 'load'")
