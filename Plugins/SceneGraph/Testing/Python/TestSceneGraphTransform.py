"""Test SceneGraph transform composition

Ensures that that all graph traversal apis are resilient to a cycle. While
the SceneGraph public api disallows creation of cycles, it is implemented in
terms of ParaView's proxy machinery, and we can't prevent people from using
that in ways that might create a cycle. But we can make sure we don't spin
forever if we encounter one.

To run directly with pvpython:

     bin/pvpython .../TestSceneGraphTransform.py
"""

from paraview.simple import LoadDistributedPlugin
from paraview.incubator import scenegraph as sg
from paraview.incubator.vtkPVIncubatorSceneGraph import vtkSMSceneGraphProxy as SG
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from vtkmodules.vtkCommonTransforms import vtkTransform
from scenegraph_testing import refuses


LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())


def translation(x, y, z):
    t = vtkTransform()
    t.Translate(x, y, z)
    m = vtkMatrix4x4()
    m.DeepCopy(t.GetMatrix())
    return m


def rotation_z(degrees):
    t = vtkTransform()
    t.RotateZ(degrees)
    m = vtkMatrix4x4()
    m.DeepCopy(t.GetMatrix())
    return m


def world_origin_of(graph, node):
    """Where the node's own origin lands in world space."""
    return graph.GetWorldTransform(node).MultiplyPoint([0.0, 0.0, 0.0, 1.0])[:3]


def close(actual, expected, tol=1e-9):
    return all(abs(a - e) < tol for a, e in zip(actual, expected))


def new_graph():
    graph = sg.create()
    return graph, graph.GetRoot()


def check_local_round_trip():
    graph, root = new_graph()
    node = graph.CreateChild(root, "Node")

    assert graph.SetLocalTransform(node, translation(1, 2, 3))

    back = graph.GetLocalTransform(node)
    assert (back.GetElement(0, 3), back.GetElement(1, 3), back.GetElement(2, 3)) == (
        1.0,
        2.0,
        3.0,
    ), "the stored matrix is not what was set"

    # An untouched node is the identity.
    other = graph.CreateChild(root, "Other")
    assert graph.GetLocalTransform(other).IsIdentity()
    assert close(world_origin_of(graph, other), (0, 0, 0))


def check_matrix_order():
    """The D6 footgun: world = parent_world * local, not the reverse."""
    graph, root = new_graph()
    table = graph.CreateChild(root, "Table")
    cup = graph.CreateChild(table, "Cup")

    # Parent spins a quarter turn about Z, and child sits one unit out along +X.
    graph.SetLocalTransform(table, rotation_z(90))
    graph.SetLocalTransform(cup, translation(1, 0, 0))

    actual = world_origin_of(graph, cup)

    # The child's offset is expressed in the parent's rotated frame, so it
    # swings round to +Y.
    assert close(actual, (0, 1, 0)), (
        "expected the child to swing to (0,1,0) with its parent, got %r. "
        "Getting (1,0,0) instead means the product is the wrong way round: the "
        "child's offset is being applied along fixed world axes." % (actual,)
    )
    assert not close(actual, (1, 0, 0)), "child did not follow its rotating parent"


def check_deep_composition():
    graph, root = new_graph()
    a = graph.CreateChild(root, "A")
    b = graph.CreateChild(a, "B")
    c = graph.CreateChild(b, "C")

    graph.SetLocalTransform(a, translation(10, 0, 0))
    graph.SetLocalTransform(b, translation(0, 5, 0))
    graph.SetLocalTransform(c, translation(0, 0, 2))

    assert close(world_origin_of(graph, a), (10, 0, 0))
    assert close(world_origin_of(graph, b), (10, 5, 0))
    assert close(world_origin_of(graph, c), (10, 5, 2)), world_origin_of(graph, c)

    # A change part-way up moves everything below it, but not things above it
    graph.SetLocalTransform(b, translation(0, 50, 0))
    assert close(world_origin_of(graph, a), (10, 0, 0)), "an ancestor should not move"
    assert close(world_origin_of(graph, c), (10, 50, 2))


def check_root_is_the_world_transform():
    """The root's local transform is the world/navigation transform, so it
    composes into every descendant."""
    graph, root = new_graph()
    a = graph.CreateChild(root, "A")
    graph.SetLocalTransform(a, translation(1, 0, 0))
    assert close(world_origin_of(graph, a), (1, 0, 0))

    graph.SetLocalTransform(root, translation(0, 0, 100))
    assert close(world_origin_of(graph, a), (1, 0, 100)), world_origin_of(graph, a)
    assert close(world_origin_of(graph, root), (0, 0, 100))


def check_reparenting_changes_world_transform():
    graph, root = new_graph()
    near = graph.CreateChild(root, "Near")
    far = graph.CreateChild(root, "Far")
    graph.SetLocalTransform(near, translation(1, 0, 0))
    graph.SetLocalTransform(far, translation(100, 0, 0))

    moved = graph.CreateChild(near, "Moved")
    graph.SetLocalTransform(moved, translation(0, 1, 0))
    assert close(world_origin_of(graph, moved), (1, 1, 0))

    # Its own local transform is untouched, but its chain is different.
    assert graph.MoveNode(moved, far)
    assert close(world_origin_of(graph, moved), (100, 1, 0)), world_origin_of(graph, moved)


def check_traverse_survives_a_cycle():
    """The public scenegraph api cannot build a cycle, but a statefile or a
    direct property edit can. Every traversing method must finish on such a graph,
    and those that encounter the repeated node must report it, which through the
    wrapper is a raise."""
    graph, root = new_graph()
    table = graph.CreateChild(root, "Table")
    cup = graph.CreateChild(table, "Cup")

    # Go around the API to make Cup a parent of its own ancestor
    cup.GetProperty("Children").AddProxy(table)

    # Now that we deliberately made a graph with a cycle, avoid calling
    # UpdateVTKObjects() since pushing a cyclic proxy-property graph causes
    # ParaView's state rebuild hang.

    # A full walk reaches Table twice and reports it.
    refuses(lambda: graph.TraverseAndApply(), SG.NotATree)
    error = refuses(lambda: graph.ValidateStructure(), SG.NotATree)
    assert str(error), "a refusal carries a message"

    # The searches below stop when they find their target, so whether they meet
    # the repeated node first depends on where the target sits in the walk. That
    # is not part of the contract, however. The only guarantee is that each search
    # finishes, and either returns its answer with no error, or raises NotATree.
    for label, search in (
        ("GetDepth", lambda: graph.GetDepth(cup)),
        ("GetPath", lambda: graph.GetPath(cup)),
        ("GetParent", lambda: graph.GetParent(cup)),
        ("IsDescendantOf", lambda: graph.IsDescendantOf(cup, root)),
        ("GetWorldTransform", lambda: graph.GetWorldTransform(cup)),
        ("GetNodesByName", lambda: graph.GetNodesByName("Cup")),
    ):
        try:
            search()
        except sg.SceneGraphError as error:
            assert error.code == SG.NotATree, "%s raised %s on a cyclic graph" % (label, error.name)
            assert str(error), "%s raised NotATree without a message" % label
        else:
            assert graph.GetErrorCode() == SG.NoError, label

    # Undo the cycle before leaving. A cyclic Children graph is also a
    # *reference* cycle, meaning the nodes involved can never be released,
    # causing leaks.
    cup.GetProperty("Children").RemoveProxy(table)
    assert graph.ValidateStructure()
    assert graph.GetErrorCode() == SG.NoError
    assert graph.GetErrorMessage() == ""


check_local_round_trip()
check_matrix_order()
check_deep_composition()
check_root_is_the_world_transform()
check_reparenting_changes_world_transform()
check_traverse_survives_a_cycle()

print("TestSceneGraphTransform: OK")
