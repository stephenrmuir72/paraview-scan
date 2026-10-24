"""Binding sources to nodes, and applying the graph to their representations.

This is the end the whole thing exists for: bind two spheres to a parent and a
child node, move the parent, and both should move -- the child by the parent's
transform composed with its own.

Run directly with pvpython; the plugin is found by name:

    bin/pvpython .../TestSceneGraphBinding.py
"""


from paraview.simple import (
    CreateRenderView,
    Delete,
    Hide,
    LoadDistributedPlugin,
    RenameSource,
    SetActiveView,
    Show,
    Sphere,
)
from paraview import servermanager as sm
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


def user_transform_origin(rep):
    """Apply the representation's UserTransform to a point at the origin"""
    prop = rep.GetProperty("UserTransform")
    m = vtkMatrix4x4()
    m.DeepCopy([prop.GetElement(i) for i in range(16)])
    return m.MultiplyPoint([0.0, 0.0, 0.0, 1.0])[:3]


def close(actual, expected, tol=1e-9):
    return all(abs(a - e) < tol for a, e in zip(actual, expected))


# Create views and spheres once, since it's expensive
VIEW = CreateRenderView()
BIG = Sphere(registrationName="Big")
SMALL = Sphere(registrationName="Small")
BIG_REP = Show(BIG, VIEW)
SMALL_REP = Show(SMALL, VIEW)
OTHER_VIEW = CreateRenderView()  # a second target
EMPTY_VIEW = CreateRenderView()  # nothing is ever shown here


def scene():
    """Return everything in the scene as a tuple"""
    for graph in sg.all_graphs():
        sg.delete(graph)
    SetActiveView(VIEW)
    return VIEW, BIG, SMALL, BIG_REP, SMALL_REP, sg.create()


def check_target_view_defaults_to_active():
    view, big, small, big_rep, small_rep, graph = scene()
    assert graph.GetTargetViews() == [view.SMProxy], (
        "a new graph should target the active render view, got %r"
        % (graph.GetTargetViews(),)
    )


def check_binding_bookkeeping():
    view, big, small, big_rep, small_rep, graph = scene()
    root = graph.GetRoot()
    table = graph.CreateChild(root, "Table")

    assert graph.GetNumberOfSources(table) == 0
    assert graph.AddSource(table, big.SMProxy, 0)
    assert graph.GetNumberOfSources(table) == 1
    assert graph.GetSource(table, 0) is big.SMProxy
    assert graph.GetSourcePort(table, 0) == 0

    # Check that a port is stored with its binding
    assert graph.AddSource(table, small.SMProxy, 1)
    ports = sorted(graph.GetSourcePort(table, i) for i in range(graph.GetNumberOfSources(table)))
    assert ports == [0, 1], ports
    assert graph.GetOwningNode(small.SMProxy, 1) is not None
    assert graph.GetOwningNode(small.SMProxy, 0) is None, "port 0 is a different binding"
    graph.RemoveSource(table, small.SMProxy, 1)

    assert graph.GetOwningNode(big.SMProxy, 0) is table
    assert graph.GetOwningNode(small.SMProxy, 0) is None

    # Check that binding a bound source to another node moves it
    other = graph.CreateChild(root, "Other")
    assert graph.AddSource(other, big.SMProxy, 0)
    assert graph.GetNumberOfSources(table) == 0, "the first node should have let it go"
    assert graph.GetNumberOfSources(other) == 1

    # Binding the same pair twice to the same node does not create a duplicate.
    assert graph.AddSource(other, big.SMProxy, 0)
    assert graph.GetNumberOfSources(other) == 1

    assert graph.RemoveSource(other, big.SMProxy, 0)
    assert graph.GetNumberOfSources(other) == 0


def check_a_foreign_node_cannot_be_bound():
    """Make sure we can't bind a source to another graph's node"""
    view, big, small, big_rep, small_rep, graph = scene()
    other = sg.create()
    stranger = other.CreateChild(other.GetRoot(), "Stranger")

    refuses(lambda: graph.AddSource(stranger, big.SMProxy, 0))
    assert other.GetNumberOfSources(stranger) == 0, "nothing should have been written"
    assert graph.GetOwningNode(big.SMProxy, 0) is None


def check_one_binding_per_source_and_view():
    """Two graphs targeting one view may not both bind the same (source, port),
    since each apply would rewrite the other's placement. Different sources in one
    view, or the same source in different views, are fine."""
    view, big, small, big_rep, small_rep, furniture = scene()
    instruments = sg.create()
    assert instruments.GetTargetViews() == [view.SMProxy], "both graphs target the view"
    table = furniture.CreateChild(furniture.GetRoot(), "Table")
    dial = instruments.CreateChild(instruments.GetRoot(), "Dial")

    assert furniture.AddSource(table, big.SMProxy, 0)
    assert instruments.AddSource(dial, small.SMProxy, 0), "a different source is fine"

    refuses(lambda: instruments.AddSource(dial, big.SMProxy, 0), SG.BindingConflict)
    assert instruments.GetNumberOfSources(dial) == 1, "nothing should have been written"
    assert furniture.GetOwningNode(big.SMProxy, 0) is table, "the first graph keeps it"
    assert instruments.AddSource(dial, big.SMProxy, 1), "another port is another binding"

    # A graph targeting a different view may bind the same source...
    other_view = OTHER_VIEW
    SetActiveView(other_view)
    third = sg.create()
    assert third.GetTargetViews() == [other_view.SMProxy]
    knob = third.CreateChild(third.GetRoot(), "Knob")
    assert third.AddSource(knob, big.SMProxy, 0)

    # ...but cannot then target the shared view
    refuses(lambda: third.AddTargetView(view.SMProxy), SG.BindingConflict)
    assert third.GetTargetViews() == [other_view.SMProxy], "a refused view must not be added"

    # Circumvent the API's guards, then ensure ValidateStructure reports
    # the conflict from each graph's side
    third.GetProperty("TargetViews").AddProxy(view.SMProxy)
    third.UpdateVTKObjects()
    refuses(lambda: third.ValidateStructure(), SG.BindingConflict)
    refuses(lambda: furniture.ValidateStructure(), SG.BindingConflict)
    third.GetProperty("TargetViews").RemoveProxy(view.SMProxy)
    third.UpdateVTKObjects()
    assert third.ValidateStructure() and furniture.ValidateStructure()


def check_apply_moves_bound_representations():
    view, big, small, big_rep, small_rep, graph = scene()
    root = graph.GetRoot()
    parent = graph.CreateChild(root, "Parent")
    child = graph.CreateChild(parent, "Child")

    graph.AddSource(parent, big.SMProxy, 0)
    graph.AddSource(child, small.SMProxy, 0)

    graph.SetLocalTransform(parent, translation(10, 0, 0))
    graph.SetLocalTransform(child, translation(0, 3, 0))

    # Nothing has been applied yet, so both are still at the origin.
    assert close(user_transform_origin(big_rep.SMProxy), (0, 0, 0)), (
        "SetLocalTransform must not touch a representation on its own"
    )

    graph.TraverseAndApply()

    assert close(user_transform_origin(big_rep.SMProxy), (10, 0, 0)), user_transform_origin(
        big_rep.SMProxy
    )
    assert close(user_transform_origin(small_rep.SMProxy), (10, 3, 0)), (
        "the child should carry its parent's transform too, got %r"
        % (user_transform_origin(small_rep.SMProxy),)
    )

    # Moving the parent moves both.
    graph.SetLocalTransform(parent, translation(100, 0, 0))
    graph.TraverseAndApply()
    assert close(user_transform_origin(big_rep.SMProxy), (100, 0, 0))
    assert close(user_transform_origin(small_rep.SMProxy), (100, 3, 0))

    # An unbound source is never touched.
    loose = Sphere(registrationName="Loose")
    loose_rep = Show(loose, view)
    graph.TraverseAndApply()
    assert close(user_transform_origin(loose_rep.SMProxy), (0, 0, 0))
    Delete(loose)


def check_apply_is_idempotent_and_picks_up_new_representations():
    view, big, small, big_rep, small_rep, graph = scene()
    root = graph.GetRoot()
    node = graph.CreateChild(root, "Node")
    graph.AddSource(node, big.SMProxy, 0)
    graph.SetLocalTransform(node, translation(5, 0, 0))

    graph.TraverseAndApply()
    first = user_transform_origin(big_rep.SMProxy)
    graph.TraverseAndApply()
    assert close(user_transform_origin(big_rep.SMProxy), first), (
        "applying twice should not change the answer"
    )

    # A representation that appears later must still be written, even though
    # the node itself has not moved since the last apply.
    second_view = OTHER_VIEW
    graph.AddTargetView(second_view.SMProxy)
    second_rep = Show(big, second_view)
    assert close(user_transform_origin(second_rep.SMProxy), (0, 0, 0))
    graph.TraverseAndApply()
    assert close(user_transform_origin(second_rep.SMProxy), (5, 0, 0)), (
        "a newly shown representation should be written on the next apply"
    )

    # Hiding it should not break the next apply.
    Hide(big, second_view)
    graph.TraverseAndApply()


def check_driven_representations():
    view, big, small, big_rep, small_rep, graph = scene()
    root = graph.GetRoot()
    node = graph.CreateChild(root, "Node")
    graph.AddSource(node, big.SMProxy, 0)

    assert graph.GetDrivenRepresentations(node, view.SMProxy) == [big_rep.SMProxy]

    # A view where the source is not shown drives nothing.
    assert graph.GetDrivenRepresentations(node, EMPTY_VIEW.SMProxy) == []


def check_only_render_views_are_targeted():
    view, big, small, big_rep, small_rep, graph = scene()
    before = graph.GetTargetViews()

    # A spreadsheet view has no UserTransform to drive, so it is refused.
    spreadsheet = sm.CreateProxy("views", "SpreadSheetView")
    refuses(lambda: graph.AddTargetView(spreadsheet))

    assert graph.GetTargetViews() == before, "a refused view must not be added"


def check_validate_structure():
    view, big, small, big_rep, small_rep, graph = scene()
    root = graph.GetRoot()
    table = graph.CreateChild(root, "Table")
    cup = graph.CreateChild(table, "Cup")
    graph.AddSource(table, big.SMProxy, 0)

    assert graph.ValidateStructure(), "a healthy graph should validate"

    # Do we detect something that is not a node at all in a child list?
    table.GetProperty("Children").AddProxy(big.SMProxy)
    refuses(lambda: graph.ValidateStructure(), SG.NotANode)
    assert graph.GetNumberOfChildren(table) == len(graph.GetChildren(table)) == 1, (
        "a child that is not a node should not be counted"
    )
    table.GetProperty("Children").RemoveProxy(big.SMProxy)
    assert graph.ValidateStructure()

    # Do we detect a binding to something that is not a pipeline source?
    table.GetProperty("Sources").AddProxy(view.SMProxy)
    refuses(lambda: graph.ValidateStructure(), SG.NotASource)
    table.GetProperty("Sources").RemoveProxy(view.SMProxy)
    assert graph.ValidateStructure()

    # Circumvent the API to create two siblings sharing a name, and make
    # sure we detect that as well.
    twin = graph.CreateChild(table, "Mug")
    assert graph.ValidateStructure()
    twin.GetProperty("Name").SetElement(0, "Cup")
    twin.UpdateVTKObjects()
    assert graph.GetNodeName(cup) == graph.GetNodeName(twin) == "Cup"
    refuses(lambda: graph.ValidateStructure(), SG.NameTaken)
    twin.GetProperty("Name").SetElement(0, "Mug")
    twin.UpdateVTKObjects()
    assert graph.ValidateStructure()

    # Assert we detect a separator in a name (requires direct property set)
    twin.GetProperty("Name").SetElement(0, "a/b")
    twin.UpdateVTKObjects()
    assert graph.FindNode(graph.GetPath(twin)) is not twin, "the path stops resolving"
    refuses(lambda: graph.ValidateStructure(), SG.NameHasSeparator)
    twin.GetProperty("Name").SetElement(0, "Mug")
    twin.UpdateVTKObjects()
    assert graph.ValidateStructure()


def check_unbinding_resets_the_transform():
    """Unbinding is the only way a user has to put an object back where it
    started, so any method that stops driving a source resets it."""
    view, big, small, big_rep, small_rep, graph = scene()
    root = graph.GetRoot()

    node = graph.CreateChild(root, "Node")
    graph.AddSource(node, big.SMProxy, 0)
    graph.SetLocalTransform(node, translation(7, 0, 0))
    graph.TraverseAndApply()
    assert close(user_transform_origin(big_rep.SMProxy), (7, 0, 0))

    graph.RemoveSource(node, big.SMProxy, 0)
    assert close(user_transform_origin(big_rep.SMProxy), (0, 0, 0)), (
        "RemoveSource should put the object back, got %r"
        % (user_transform_origin(big_rep.SMProxy),)
    )

    # Deleting the node that holds a binding does the same.
    doomed = graph.CreateChild(root, "Doomed")
    graph.AddSource(doomed, big.SMProxy, 0)
    graph.SetLocalTransform(doomed, translation(0, 7, 0))
    graph.TraverseAndApply()
    assert close(user_transform_origin(big_rep.SMProxy), (0, 7, 0))
    graph.DeleteNode(doomed)
    assert close(user_transform_origin(big_rep.SMProxy), (0, 0, 0)), "DeleteNode should reset"

    # So does removing a whole subtree.
    branch = graph.CreateChild(root, "Branch")
    leaf = graph.CreateChild(branch, "Leaf")
    graph.AddSource(leaf, big.SMProxy, 0)
    graph.SetLocalTransform(leaf, translation(0, 0, 7))
    graph.TraverseAndApply()
    assert close(user_transform_origin(big_rep.SMProxy), (0, 0, 7))
    graph.RemoveSubtree(branch)
    assert close(user_transform_origin(big_rep.SMProxy), (0, 0, 0)), "RemoveSubtree should reset"

    # And so does dropping the view the object was being driven in.
    kept = graph.CreateChild(root, "Kept")
    graph.AddSource(kept, big.SMProxy, 0)
    graph.SetLocalTransform(kept, translation(4, 4, 4))
    graph.TraverseAndApply()
    assert close(user_transform_origin(big_rep.SMProxy), (4, 4, 4))
    graph.RemoveTargetView(view.SMProxy)
    assert close(user_transform_origin(big_rep.SMProxy), (0, 0, 0)), (
        "RemoveTargetView should reset what it stops driving"
    )


def check_deleting_a_source_unbinds_it():
    """Nothing stops a bound source from being deleted, make sure the graph
    notices that and removes the binding."""
    view, _, _, _, _, graph = scene()
    big = Sphere(registrationName="Doomed")
    small = Sphere(registrationName="Kept")
    Show(big, view)
    small_rep = Show(small, view)
    node = graph.CreateChild(graph.GetRoot(), "Node")
    graph.AddSource(node, big.SMProxy, 0)
    graph.AddSource(node, small.SMProxy, 0)
    assert graph.GetNumberOfSources(node) == 2

    # A rename triggers the same "unregister" event a delete does. Make
    # sure the renamed source is still bound afterwards.
    RenameSource("Renamed", big)
    assert graph.GetNumberOfSources(node) == 2, (
        "renaming a bound source should not drop its binding, got %d bindings"
        % graph.GetNumberOfSources(node)
    )
    assert graph.GetOwningNode(big.SMProxy, 0) is node, "the renamed source is still bound"

    Delete(big)
    del big

    assert graph.GetNumberOfSources(node) == 1, (
        "deleting a bound source should drop its binding, got %d bindings"
        % graph.GetNumberOfSources(node)
    )
    assert graph.GetSource(node, 0) is small.SMProxy, (
        "the surviving binding should be the one that was not deleted"
    )
    assert graph.ValidateStructure()

    # And the graph still applies cleanly afterwards.
    graph.SetLocalTransform(node, translation(2, 0, 0))
    graph.TraverseAndApply()
    assert close(user_transform_origin(small_rep.SMProxy), (2, 0, 0))
    Delete(small)


check_target_view_defaults_to_active()
check_unbinding_resets_the_transform()
check_deleting_a_source_unbinds_it()
check_binding_bookkeeping()
check_a_foreign_node_cannot_be_bound()
check_one_binding_per_source_and_view()
check_apply_moves_bound_representations()
check_apply_is_idempotent_and_picks_up_new_representations()
check_driven_representations()
check_only_render_views_are_targeted()
check_validate_structure()

print("TestSceneGraphBinding: OK")
