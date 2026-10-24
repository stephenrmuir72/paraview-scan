"""Every public call leaves GetErrorCode() describing that call.

While other tests assert what a call returns, this one ensures error codes
are set correctly by calls that encounter any error case. In the case a
call encounters an error, the error code should indicate the category of
error, and the message should not be empty. Successful calls with no error
should set the code to NoError and the message should be empty.

This is the C++ contract, so here calls are made on the bare proxy. The
Python wrapper turns these codes into exceptions and is tested separately in
TestSceneGraphWrapper.py.

Run directly with pvpython; the plugin is found by name:

    bin/pvpython .../TestSceneGraphErrorCodes.py
"""

from paraview.simple import CreateRenderView, LoadDistributedPlugin, SetActiveView, Show, Sphere
from paraview import servermanager as sm
from paraview.incubator import scenegraph as sg
from paraview.incubator.vtkPVIncubatorSceneGraph import vtkSMSceneGraphProxy as SG
from vtkmodules.vtkCommonCore import vtkCollection
from vtkmodules.vtkCommonMath import vtkMatrix4x4


LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())
failures = []


def expect(label, graph, call, code):
    """Invoke the call, then check the code and message it left on the graph."""
    call()
    got = graph.GetErrorCode()
    message = graph.GetErrorMessage()
    if got != code:
        failures.append("%-44s expected code %d, got %d (%r)" % (label, code, got, message))
    elif code == SG.NoError and message:
        failures.append("%-44s NoError but a message was left: %r" % (label, message))
    elif code != SG.NoError and not message:
        failures.append("%-44s code %d but no message" % (label, code))


# Create scene proxies once, since it's expensive
VIEW = CreateRenderView()
SPHERE = Sphere(registrationName="Bound")
Show(SPHERE, VIEW)
SPARE_VIEW = CreateRenderView()


def scene():
    # Remove the previous check's graphs, then create:
    #   - a graph containing: World -> Table -> {Cup, Plate}, targeting VIEW
    #   - a second graph containing: World -> Stranger
    for graph in sg.all_graphs():
        sg.delete(graph)
    SetActiveView(VIEW)
    graph = sg.create().SMProxy
    root = graph.GetRoot()
    table = graph.CreateChild(root, "Table")
    cup = graph.CreateChild(table, "Cup")
    graph.CreateChild(table, "Plate")
    other = sg.create().SMProxy
    stranger = other.CreateChild(other.GetRoot(), "Stranger")
    return graph, root, table, cup, stranger, SPHERE.SMProxy, VIEW.SMProxy, SPARE_VIEW.SMProxy


def check_successes_leave_no_error():
    graph, root, table, cup, stranger, sphere, view, spare = scene()
    m = vtkMatrix4x4()
    c = vtkCollection()
    x = graph.CreateChild(table, "X")
    expect("GetParent(cup)", graph, lambda: graph.GetParent(cup), SG.NoError)
    expect("GetParent(root) is a legitimate null", graph, lambda: graph.GetParent(root), SG.NoError)
    expect("GetDepth(cup)", graph, lambda: graph.GetDepth(cup), SG.NoError)
    expect("GetPath(cup)", graph, lambda: graph.GetPath(cup), SG.NoError)
    expect("FindNode(missing) is legitimate", graph, lambda: graph.FindNode("Nope"), SG.NoError)
    expect("FindChild(missing) is legitimate", graph, lambda: graph.FindChild(table, "zz"), SG.NoError)
    expect("GetNumberOfChildren(table)", graph, lambda: graph.GetNumberOfChildren(table), SG.NoError)
    expect("GetChild(table, 0)", graph, lambda: graph.GetChild(table, 0), SG.NoError)
    expect("GetChildren(table)", graph, lambda: graph.GetChildren(table, c), SG.NoError)
    expect("GetNodesByName", graph, lambda: graph.GetNodesByName("Cup", c), SG.NoError)
    expect("IsDescendantOf(cup, root)", graph, lambda: graph.IsDescendantOf(cup, root), SG.NoError)
    expect("IsDescendantOf(root, cup) is false", graph, lambda: graph.IsDescendantOf(root, cup), SG.NoError)
    expect("GetNodeName(cup)", graph, lambda: graph.GetNodeName(cup), SG.NoError)
    expect("MoveNode(x, root)", graph, lambda: graph.MoveNode(x, root), SG.NoError)
    expect("MoveNode same parent, no-op", graph, lambda: graph.MoveNode(x, root), SG.NoError)
    expect("RenameNode(x)", graph, lambda: graph.RenameNode(x, "Y"), SG.NoError)
    expect("RenameNode(root)", graph, lambda: graph.RenameNode(root, "World"), SG.NoError)
    expect("SetLocalTransform(x)", graph, lambda: graph.SetLocalTransform(x, m), SG.NoError)
    expect("GetLocalTransform(x)", graph, lambda: graph.GetLocalTransform(x, m), SG.NoError)
    expect("GetWorldTransform(x)", graph, lambda: graph.GetWorldTransform(x, m), SG.NoError)
    expect("AddSource(x)", graph, lambda: graph.AddSource(x, sphere), SG.NoError)
    expect("AddSource again, already bound", graph, lambda: graph.AddSource(x, sphere), SG.NoError)
    expect("GetNumberOfSources(x)", graph, lambda: graph.GetNumberOfSources(x), SG.NoError)
    expect("GetSource(x, 0)", graph, lambda: graph.GetSource(x, 0), SG.NoError)
    expect("GetSourcePort(x, 0)", graph, lambda: graph.GetSourcePort(x, 0), SG.NoError)
    expect("GetOwningNode(sphere)", graph, lambda: graph.GetOwningNode(sphere), SG.NoError)
    expect("GetOwningNode(unbound port) is null", graph, lambda: graph.GetOwningNode(sphere, 7), SG.NoError)
    expect("GetDrivenRepresentations", graph, lambda: graph.GetDrivenRepresentations(x, view, c), SG.NoError)
    expect("GetTargetViews", graph, lambda: graph.GetTargetViews(c), SG.NoError)
    expect("AddTargetView already targeted", graph, lambda: graph.AddTargetView(view), SG.NoError)
    expect("TraverseAndApply", graph, lambda: graph.TraverseAndApply(), SG.NoError)
    expect("ValidateStructure", graph, lambda: graph.ValidateStructure(), SG.NoError)
    expect("RemoveSource(x)", graph, lambda: graph.RemoveSource(x, sphere), SG.NoError)
    expect("DeleteNode(x)", graph, lambda: graph.DeleteNode(x), SG.NoError)
    expect("RemoveSubtree(table)", graph, lambda: graph.RemoveSubtree(table), SG.NoError)
    expect("RemoveTargetView(view)", graph, lambda: graph.RemoveTargetView(view), SG.NoError)
    # Readers do not check membership, so a foreign node is not a refusal there.
    expect("GetNodeName(foreign) reads", graph, lambda: graph.GetNodeName(stranger), SG.NoError)
    expect("GetNumberOfChildren(foreign) reads", graph, lambda: graph.GetNumberOfChildren(stranger), SG.NoError)
    expect("GetLocalTransform(foreign) reads", graph, lambda: graph.GetLocalTransform(stranger, m), SG.NoError)


def check_null_arguments():
    graph, root, table, cup, stranger, sphere, view, spare = scene()
    m = vtkMatrix4x4()
    c = vtkCollection()
    N = SG.NullArgument
    expect("GetNodeName(None)", graph, lambda: graph.GetNodeName(None), N)
    expect("GetNumberOfChildren(None)", graph, lambda: graph.GetNumberOfChildren(None), N)
    expect("GetChild(None, 0)", graph, lambda: graph.GetChild(None, 0), N)
    expect("GetChildren(None, c)", graph, lambda: graph.GetChildren(None, c), N)
    expect("GetChildren(table, None)", graph, lambda: graph.GetChildren(table, None), N)
    expect("GetParent(None)", graph, lambda: graph.GetParent(None), N)
    expect("IsDescendantOf(None, root)", graph, lambda: graph.IsDescendantOf(None, root), N)
    expect("IsDescendantOf(cup, None)", graph, lambda: graph.IsDescendantOf(cup, None), N)
    expect("GetDepth(None)", graph, lambda: graph.GetDepth(None), N)
    expect("GetPath(None)", graph, lambda: graph.GetPath(None), N)
    expect("FindChild(None, name)", graph, lambda: graph.FindChild(None, "Cup"), N)
    expect("FindChild(table, None)", graph, lambda: graph.FindChild(table, None), N)
    expect("FindNode(None)", graph, lambda: graph.FindNode(None), N)
    expect("GetNodesByName(None, c)", graph, lambda: graph.GetNodesByName(None, c), N)
    expect("GetNodesByName(name, None)", graph, lambda: graph.GetNodesByName("Cup", None), N)
    expect("CreateChild(None, name)", graph, lambda: graph.CreateChild(None, "A"), N)
    expect("MoveNode(None, root)", graph, lambda: graph.MoveNode(None, root), N)
    expect("MoveNode(cup, None)", graph, lambda: graph.MoveNode(cup, None), N)
    expect("RenameNode(None, name)", graph, lambda: graph.RenameNode(None, "A"), N)
    expect("RenameNode(cup, None)", graph, lambda: graph.RenameNode(cup, None), N)
    expect("RenameNode(cup, '')", graph, lambda: graph.RenameNode(cup, ""), N)
    expect("DeleteNode(None)", graph, lambda: graph.DeleteNode(None), N)
    expect("RemoveSubtree(None)", graph, lambda: graph.RemoveSubtree(None), N)
    expect("SetLocalTransform(None, m)", graph, lambda: graph.SetLocalTransform(None, m), N)
    expect("SetLocalTransform(cup, None)", graph, lambda: graph.SetLocalTransform(cup, None), N)
    expect("GetLocalTransform(None, m)", graph, lambda: graph.GetLocalTransform(None, m), N)
    expect("GetWorldTransform(None, m)", graph, lambda: graph.GetWorldTransform(None, m), N)
    expect("GetWorldTransform(cup, None)", graph, lambda: graph.GetWorldTransform(cup, None), N)
    expect("AddSource(None, sphere)", graph, lambda: graph.AddSource(None, sphere), N)
    expect("AddSource(cup, None)", graph, lambda: graph.AddSource(cup, None), N)
    expect("RemoveSource(None, sphere)", graph, lambda: graph.RemoveSource(None, sphere), N)
    expect("GetNumberOfSources(None)", graph, lambda: graph.GetNumberOfSources(None), N)
    expect("GetSource(None, 0)", graph, lambda: graph.GetSource(None, 0), N)
    expect("GetSourcePort(None, 0)", graph, lambda: graph.GetSourcePort(None, 0), N)
    expect("GetOwningNode(None)", graph, lambda: graph.GetOwningNode(None), N)
    expect("GetDrivenRepresentations(None, view, c)", graph, lambda: graph.GetDrivenRepresentations(None, view, c), N)
    expect("GetDrivenRepresentations(cup, view, None)", graph, lambda: graph.GetDrivenRepresentations(cup, view, None), N)
    expect("AddTargetView(None)", graph, lambda: graph.AddTargetView(None), N)
    expect("RemoveTargetView(None)", graph, lambda: graph.RemoveTargetView(None), N)
    expect("GetTargetViews(None)", graph, lambda: graph.GetTargetViews(None), N)
    # The statics report through the graph when there is one.
    pxm = sm.ActiveConnection.Session.GetSessionProxyManager()
    expect("DeleteSceneGraph(None, graph)", graph, lambda: SG.DeleteSceneGraph(None, graph), N)
    expect("RenameSceneGraph(pxm, graph, '')", graph, lambda: SG.RenameSceneGraph(pxm, graph, ""), N)
    expect("RenameSceneGraph(None, graph, name)", graph, lambda: SG.RenameSceneGraph(None, graph, "A"), N)


def check_foreign_nodes():
    graph, root, table, cup, stranger, sphere, view, spare = scene()
    m = vtkMatrix4x4()
    F = SG.NotInGraph
    expect("GetParent(foreign)", graph, lambda: graph.GetParent(stranger), F)
    expect("GetDepth(foreign)", graph, lambda: graph.GetDepth(stranger), F)
    expect("GetPath(foreign)", graph, lambda: graph.GetPath(stranger), F)
    expect("GetWorldTransform(foreign)", graph, lambda: graph.GetWorldTransform(stranger, m), F)
    expect("CreateChild(foreign, name)", graph, lambda: graph.CreateChild(stranger, "A"), F)
    expect("MoveNode(foreign, root)", graph, lambda: graph.MoveNode(stranger, root), F)
    expect("MoveNode(cup, foreign)", graph, lambda: graph.MoveNode(cup, stranger), F)
    expect("RenameNode(foreign)", graph, lambda: graph.RenameNode(stranger, "A"), F)
    expect("DeleteNode(foreign)", graph, lambda: graph.DeleteNode(stranger), F)
    expect("RemoveSubtree(foreign)", graph, lambda: graph.RemoveSubtree(stranger), F)
    expect("SetLocalTransform(foreign)", graph, lambda: graph.SetLocalTransform(stranger, m), F)
    expect("AddSource(foreign)", graph, lambda: graph.AddSource(stranger, sphere), F)


def check_other_refusals():
    graph, root, table, cup, stranger, sphere, view, spare = scene()
    spreadsheet = sm.CreateProxy("views", "SpreadSheetView")
    expect("MoveNode(root, table)", graph, lambda: graph.MoveNode(root, table), SG.RootIsFixed)
    expect("DeleteNode(root)", graph, lambda: graph.DeleteNode(root), SG.RootIsFixed)
    expect("RemoveSubtree(root)", graph, lambda: graph.RemoveSubtree(root), SG.RootIsFixed)
    expect("MoveNode(table, cup)", graph, lambda: graph.MoveNode(table, cup), SG.WouldFormCycle)
    expect("MoveNode(table, table)", graph, lambda: graph.MoveNode(table, table), SG.WouldFormCycle)
    expect("RenameNode(cup, sibling's name)", graph, lambda: graph.RenameNode(cup, "Plate"), SG.NameTaken)
    expect("RenameNode(cup, 'a/b')", graph, lambda: graph.RenameNode(cup, "a/b"), SG.NameHasSeparator)
    expect("GetChild(table, 99)", graph, lambda: graph.GetChild(table, 99), SG.OutOfRange)
    expect("GetSource(cup, 99)", graph, lambda: graph.GetSource(cup, 99), SG.OutOfRange)
    expect("GetSourcePort(cup, 99)", graph, lambda: graph.GetSourcePort(cup, 99), SG.OutOfRange)
    expect("AddSource(cup, sphere, -1)", graph, lambda: graph.AddSource(cup, sphere, -1), SG.OutOfRange)
    expect("RemoveSource unbound", graph, lambda: graph.RemoveSource(cup, sphere), SG.NotBound)
    expect("RemoveTargetView untargeted", graph, lambda: graph.RemoveTargetView(spare), SG.NotBound)
    expect("AddTargetView(spreadsheet)", graph, lambda: graph.AddTargetView(spreadsheet), SG.NotARenderView)


def check_not_a_tree():
    graph, root, table, cup, stranger, sphere, view, spare = scene()
    # Circumvent the api to create a node listed under two parents
    root.GetProperty("Children").AddProxy(cup)
    expect("ValidateStructure on a diamond", graph, lambda: graph.ValidateStructure(), SG.NotATree)
    expect("TraverseAndApply on a diamond", graph, lambda: graph.TraverseAndApply(), SG.NotATree)
    expect("GetNodesByName walks everything", graph, lambda: graph.GetNodesByName("zz", vtkCollection()), SG.NotATree)
    # Searching for a node from another graph walks the whole tree, so these
    # should encounter the repeat whatever order the walk takes
    expect("RenameNode(foreign) on a diamond", graph, lambda: graph.RenameNode(stranger, "A"), SG.NotATree)
    expect("MoveNode(foreign) on a diamond", graph, lambda: graph.MoveNode(stranger, table), SG.NotATree)
    expect("DeleteNode(foreign) on a diamond", graph, lambda: graph.DeleteNode(stranger), SG.NotATree)
    expect("RemoveSubtree(foreign) on a diamond", graph, lambda: graph.RemoveSubtree(stranger), SG.NotATree)
    expect("CreateChild(foreign) on a diamond", graph, lambda: graph.CreateChild(stranger, "A"), SG.NotATree)
    expect("MoveNode(_, foreign) on a diamond", graph, lambda: graph.MoveNode(table, stranger), SG.NotATree)
    expect("SetLocalTransform(foreign) on a diamond", graph,
           lambda: graph.SetLocalTransform(stranger, vtkMatrix4x4()), SG.NotATree)
    expect("AddSource(foreign) on a diamond", graph, lambda: graph.AddSource(stranger, sphere), SG.NotATree)
    root.GetProperty("Children").RemoveProxy(cup)

    # A repeat inside the subtree being moved is found by the cycle check,
    # before anything is moved
    shelf = graph.CreateChild(root, "Shelf")
    plate = graph.FindChild(table, "Plate")
    cup.GetProperty("Children").AddProxy(plate)
    expect("MoveNode with a repeat below the node", graph, lambda: graph.MoveNode(table, shelf), SG.NotATree)
    cup.GetProperty("Children").RemoveProxy(plate)
    if graph.GetParent(table) != root:
        failures.append("MoveNode with a repeat below the node moved it anyway")
    graph.DeleteNode(shelf)
    expect("ValidateStructure repaired", graph, lambda: graph.ValidateStructure(), SG.NoError)

    # When there are several defects at once, the code should convey the most
    # severe error (structural)
    root.GetProperty("Children").AddProxy(cup)
    table.GetProperty("Sources").AddProxy(view)
    expect("ValidateStructure ranks a diamond above a bad binding", graph,
           lambda: graph.ValidateStructure(), SG.NotATree)
    root.GetProperty("Children").RemoveProxy(cup)
    expect("ValidateStructure with only the bad binding", graph,
           lambda: graph.ValidateStructure(), SG.NotASource)
    table.GetProperty("Sources").RemoveProxy(view)
    expect("ValidateStructure repaired again", graph, lambda: graph.ValidateStructure(), SG.NoError)


check_successes_leave_no_error()
check_null_arguments()
check_foreign_nodes()
check_other_refusals()
check_not_a_tree()

if failures:
    for line in failures:
        print("VIOLATED  " + line)
    raise SystemExit("%d error-code violation(s)" % len(failures))

print("TestSceneGraphErrorCodes: OK")
