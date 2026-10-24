"""Validate properties that must hold for every node of any graph

The other tests assert what a named call does. These assert relationships
between calls, e.g. that a path resolves back to the node it came from, that a
child's parent is the parent it was taken from, that a world transform really is
the parent's composed with the local one.

Run directly with pvpython; the plugin is found by name:

    bin/pvpython .../TestSceneGraphInvariants.py
"""

from paraview.simple import LoadDistributedPlugin
from paraview.incubator import scenegraph as sg
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from vtkmodules.vtkCommonTransforms import vtkTransform

LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())

# Names a caller might reasonably try, including several that used to break
# path addressing.
AWKWARD_NAMES = [
    "Plain", "with space", "a/b", "..", ".", "World", "a//b", "trailing/",
    "Ünïcödé", "x" * 80,
]

failures = []


def check(prop, ok, detail):
    if not ok:
        failures.append("%-46s %s" % (prop, detail))


def transform(x, y, z, degrees):
    t = vtkTransform()
    t.RotateZ(degrees)
    t.Translate(x, y, z)
    m = vtkMatrix4x4()
    m.DeepCopy(t.GetMatrix())
    return m


def every_node(graph, node=None):
    node = graph.GetRoot() if node is None else node
    yield node
    for child in graph.GetChildren(node):
        yield from every_node(graph, child)


def build(names):
    """A three-level tree whose leaves are named from `names`, with a rotation
    somewhere above them so the transform property has something to catch."""
    graph = sg.create()
    root = graph.GetRoot()
    left = graph.CreateChild(root, "Left")
    right = graph.CreateChild(root, "Right")
    graph.SetLocalTransform(left, transform(3, 0, 0, 30))
    graph.SetLocalTransform(right, transform(0, 4, 0, -15))
    for i, name in enumerate(names):
        leaf = graph.CreateChild(left if i % 2 == 0 else right, name)
        graph.SetLocalTransform(leaf, transform(1, 1, 1, 10))
    return graph


def check_tree(label, graph):
    for node in every_node(graph):
        name = graph.GetNodeName(node)
        path = graph.GetPath(node)
        where = "%s: name=%r path=%r" % (label, name, path)

        check("a path resolves back to its own node",
              graph.FindNode(path) is node, where)
        check("depth agrees with the path",
              graph.GetDepth(node) == path.count("/"),
              "%s depth=%d" % (where, graph.GetDepth(node)))
        check("a name carries no path separator", "/" not in name, where)

        children = graph.GetChildren(node)
        check("GetNumberOfChildren agrees with GetChildren()",
              graph.GetNumberOfChildren(node) == len(children),
              "%s %d vs %d" % (where, graph.GetNumberOfChildren(node), len(children)))

        for i, child in enumerate(children):
            check("a child's parent is its parent",
                  graph.GetParent(child) is node, where)
            check("GetChild(i) agrees with GetChildren()[i]",
                  graph.GetChild(node, i) is child, "%s index=%d" % (where, i))
            check("a child is findable by name under its parent",
                  graph.FindChild(node, graph.GetNodeName(child)) is child, where)
            check("a child is a descendant of its parent",
                  graph.IsDescendantOf(child, node), where)

        parent = graph.GetParent(node)
        if parent is not None:
            world = graph.GetWorldTransform(node)
            expected = vtkMatrix4x4()
            vtkMatrix4x4.Multiply4x4(graph.GetWorldTransform(parent),
                                     graph.GetLocalTransform(node), expected)
            check("world == parent's world * local",
                  all(abs(world.GetElement(r, c) - expected.GetElement(r, c)) < 1e-12
                      for r in range(4) for c in range(4)), where)

    check("the graph validates", graph.ValidateStructure(), label)


def check_properties_survive_mutation():
    """Every property above must still hold after the tree has been edited,
    since a move rewrites names and re-chains transforms."""
    graph = build(AWKWARD_NAMES)
    left = graph.FindNode("World/Left")
    right = graph.FindNode("World/Right")

    for node in graph.GetChildren(left):
        graph.MoveNode(node, right)
    check_tree("after moving every child of Left into Right", graph)

    for node in graph.GetChildren(right)[:3]:
        graph.RenameNode(node, "Renamed_%s" % graph.GetNodeName(node)[:6])
    check_tree("after renaming", graph)

    graph.DeleteNode(right)
    check_tree("after deleting a node with children", graph)


check_tree("as built", build(AWKWARD_NAMES))
check_properties_survive_mutation()

if failures:
    for line in failures:
        print("VIOLATED  " + line)
    raise SystemExit("%d property violation(s)" % len(failures))

print("TestSceneGraphInvariants: OK")
