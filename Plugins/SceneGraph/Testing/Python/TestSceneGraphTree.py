"""Test SceneGraph tree operations

Tests building, addressing, moving, and removing nodes, and the invariants
those operations are supposed to enforce.

 To run directly with pvpython:

     bin/pvpython .../TestSceneGraphTree.py
"""

from paraview.simple import LoadDistributedPlugin
from paraview import servermanager as sm
from paraview.incubator import scenegraph as sg
from scenegraph_testing import refuses


LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())
pxm = sm.ActiveConnection.Session.GetSessionProxyManager()


def child_names(graph, node):
    return [graph.GetNodeName(child) for child in graph.GetChildren(node)]


def build():
    """World -> {Table -> {Cup, Plate}, Lamp}"""
    graph = sg.create()
    root = graph.GetRoot()
    table = graph.CreateChild(root, "Table")
    cup = graph.CreateChild(table, "Cup")
    plate = graph.CreateChild(table, "Plate")
    lamp = graph.CreateChild(root, "Lamp")
    return graph, root, table, cup, plate, lamp


def check_structure():
    graph, root, table, cup, plate, lamp = build()

    assert graph.GetNumberOfChildren(root) == 2, "root should have 2 children"
    assert child_names(graph, root) == ["Table", "Lamp"], "child order not preserved"
    assert child_names(graph, table) == ["Cup", "Plate"]

    assert graph.GetChild(root, 0) is table

    assert graph.GetParent(cup) is table, "GetParent should find the real parent"
    assert graph.GetParent(table) is root
    assert graph.GetParent(root) is None, "the root has no parent"

    assert graph.GetDepth(root) == 0
    assert graph.GetDepth(table) == 1
    assert graph.GetDepth(cup) == 2

    assert graph.IsDescendantOf(cup, root), "cup is under root"
    assert not graph.IsDescendantOf(root, cup), "root is not under cup"
    assert not graph.IsDescendantOf(cup, cup), "a node is not its own descendant"


def check_addressing():
    graph, root, table, cup, plate, lamp = build()

    assert graph.FindNode("World/Table/Cup") is cup
    assert graph.FindNode("World") is root

    # A path must begin with the root's name
    for path in ("Table/Cup", "/Table/Cup", ""):
        assert graph.FindNode(path) is None, "FindNode(%r) should find nothing" % path

    assert graph.FindNode("World/Table/Missing") is None
    assert graph.FindNode("World/Nope/Cup") is None

    # The root may share a name with one of its children
    graph.RenameNode(root, "Table")
    assert graph.FindNode("Table") is root
    assert graph.FindNode("Table/Table/Cup") is cup
    graph.RenameNode(root, "World")

    assert graph.GetPath(cup) == "World/Table/Cup", graph.GetPath(cup)
    assert graph.GetPath(root) == "World"

    assert graph.FindChild(table, "Plate") is plate
    assert graph.FindChild(table, "Lamp") is None, "FindChild only looks one level down"

    # Only siblings must differ, so the same name can appear in two branches.
    other_cup = graph.CreateChild(lamp, "Cup")
    found = graph.GetNodesByName("Cup")
    # Order is whatever the traversal happens to produce, so compare as a set.
    assert set(found) == {cup, other_cup}, (
        "expected both nodes named Cup, got %d" % len(found)
    )


def check_name_uniqueness():
    graph, root, table, cup, plate, lamp = build()

    # Attaching makes the name unique rather than failing.
    second = graph.CreateChild(table, "Cup")
    assert second is not None, "CreateChild should not fail on a name collision"
    assert child_names(graph, table) == ["Cup", "Plate", "Cup1"], child_names(graph, table)

    third = graph.CreateChild(table, "Cup")
    assert child_names(graph, table) == ["Cup", "Plate", "Cup1", "Cup2"]

    # Renaming to a taken name fails and leaves the name alone.
    refuses(lambda: graph.RenameNode(plate, "Cup"))
    assert graph.GetNodeName(plate) == "Plate", "a failed rename must not change the name"

    # Renaming to a free name succeeds.
    assert graph.RenameNode(plate, "Saucer")
    assert graph.GetNodeName(plate) == "Saucer"

    # Renaming a node to its own current name is not a collision with itself.
    assert graph.RenameNode(plate, "Saucer")

    # A name is free again in a different parent.
    assert graph.RenameNode(lamp, "Cup"), "Lamp's siblings are root's children"


def check_names_cannot_contain_the_path_separator():
    graph, root, table, cup, plate, lamp = build()

    # Renaming is refused outright, same as for a collision
    refuses(lambda: graph.RenameNode(cup, "a/b"))
    assert graph.GetNodeName(cup) == "Cup", "a failed rename must not change the name"

    # Create sanitizes instead of failing
    odd = graph.CreateChild(table, "a/b")
    assert odd is not None
    assert "/" not in graph.GetNodeName(odd), graph.GetNodeName(odd)


def check_a_foreign_node_is_refused():
    """Make sure the API does not let one graph reach into another"""
    graph, root, table, cup, plate, lamp = build()
    other, _, other_table, other_cup, _, _ = build()

    # The other graph refuses this because Cup's sibling is already Plate.
    refuses(lambda: other.RenameNode(other_cup, "Plate"))
    # Our graph refuses to rename it out from under another graph.
    refuses(lambda: graph.RenameNode(other_cup, "Plate"))
    assert other.GetNodeName(other_cup) == "Cup"
    assert other.ValidateStructure(), "the other graph must be left sound"

    # Ensure a node may not be moved into another graph's tree.
    refuses(lambda: graph.MoveNode(cup, other_table))
    assert graph.GetParent(cup) is table, "the failed move must change nothing"
    assert child_names(other, other_table) == ["Cup", "Plate"], "the other tree is untouched"

    # Ensure a child may not be created under another graph's node.
    refuses(lambda: graph.CreateChild(other_table, "Intruder"))
    assert child_names(other, other_table) == ["Cup", "Plate"], "the other tree is untouched"

    assert graph.ValidateStructure() and other.ValidateStructure()


def check_a_removed_node_cannot_come_back():
    """RemoveSubtree unregisters the proxies for all descendant nodes, but a
    caller may still hold the handle. Allowing them to put it back would leave
    the tree referencing a node with no registration, and a saved statefile
    would then hold a child reference to an invalid proxy."""
    graph, root, table, cup, plate, lamp = build()

    graph.RemoveSubtree(table)
    assert pxm.GetProxyName("scenegraph_nodes", table) is None, "RemoveSubtree unregisters"

    refuses(lambda: graph.MoveNode(table, root))
    assert child_names(graph, root) == ["Lamp"], "the tree must be unchanged"
    assert graph.ValidateStructure()


def check_move():
    graph, root, table, cup, plate, lamp = build()

    assert graph.MoveNode(cup, lamp), "moving Cup under Lamp should succeed"
    assert graph.GetParent(cup) is lamp
    assert child_names(graph, table) == ["Plate"], "Cup should be gone from Table"
    assert child_names(graph, lamp) == ["Cup"]
    assert graph.GetPath(cup) == "World/Lamp/Cup"

    # Cycles are rejected by MoveNode
    refuses(lambda: graph.MoveNode(table, plate))
    assert graph.GetParent(table) is root, "the failed move must change nothing"

    # A move to the parent the node already has is a no-op, not a trip to the
    # end of the list
    graph.CreateChild(lamp, "After")
    assert child_names(graph, lamp) == ["Cup", "After"]
    assert graph.MoveNode(cup, lamp), "moving a node to its own parent should succeed"
    assert child_names(graph, lamp) == ["Cup", "After"], "a same-parent move must not reorder"

    # Moving into a parent that already uses the name uniquifies the *moved*
    # node, so Table gains a second, unrelated node called Cup.
    graph.CreateChild(table, "Cup")
    assert graph.MoveNode(cup, table)
    assert child_names(graph, table) == ["Plate", "Cup", "Cup1"], child_names(graph, table)
    assert graph.GetNodeName(cup) == "Cup1"


def check_delete_splices_children():
    graph, root, table, cup, plate, lamp = build()

    # Table is not the last child, so where its children land is visible.
    assert child_names(graph, root) == ["Table", "Lamp"]

    before = pxm.GetNumberOfProxies("scenegraph_nodes")
    assert graph.DeleteNode(table), "DeleteNode should succeed"

    # Table's children take its place, in order, at its former position
    assert child_names(graph, root) == ["Cup", "Plate", "Lamp"], child_names(graph, root)
    assert graph.GetParent(cup) is root
    assert graph.GetPath(cup) == "World/Cup"

    # The node itself is gone from the registry, but its children are not.
    after = pxm.GetNumberOfProxies("scenegraph_nodes")
    assert after == before - 1, "expected exactly one node to be unregistered"


def check_delete_renames_on_collision():
    graph, root, table, cup, plate, lamp = build()

    # Root already has a child named Cup, but deleting Table moves another Cup
    # into the same parent, so one of them has to be renamed.
    graph.CreateChild(root, "Cup")
    assert graph.DeleteNode(table)
    names = child_names(graph, root)
    assert len(names) == len(set(names)), "siblings must stay unique: %r" % names


def check_remove_subtree():
    graph, root, table, cup, plate, lamp = build()

    before = pxm.GetNumberOfProxies("scenegraph_nodes")
    assert graph.RemoveSubtree(table), "RemoveSubtree should succeed"

    assert child_names(graph, root) == ["Lamp"], "Table should be gone"
    after = pxm.GetNumberOfProxies("scenegraph_nodes")
    assert after == before - 3, (
        "expected Table, Cup and Plate to be unregistered (%d -> %d)" % (before, after)
    )


check_structure()
check_addressing()
check_name_uniqueness()
check_names_cannot_contain_the_path_separator()
check_a_foreign_node_is_refused()
check_a_removed_node_cannot_come_back()
check_move()
check_delete_splices_children()
check_delete_renames_on_collision()
check_remove_subtree()

print("TestSceneGraphTree: OK")
