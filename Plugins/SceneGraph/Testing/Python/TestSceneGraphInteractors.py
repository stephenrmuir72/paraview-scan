"""Tests the example grab style, driven by synthetic events against a real graph.

VR events are C++ structs the Python wrapping cannot build, and the CAVE event
loop only runs with a hardware connection, so this drives the style's handlers
directly with the arguments the Python style proxy would hand them: a role
name and a row-major 16-element list for a tracker, a role and a state for a
button. A stand-in plays the part of the style proxy, supplying the navigation
matrix. Between events the graph is applied the way the CAVE loop applies it,
once per frame, and the effect is read back from the bound representations'
UserTransform.
"""
import importlib.util
import os
import sys

# keep __pycache__ out of the examples directory
sys.dont_write_bytecode = True

from paraview.simple import CreateRenderView, LoadDistributedPlugin, Show, Sphere
from paraview.incubator import scenegraph as sg
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from vtkmodules.vtkCommonTransforms import vtkTransform


LoadDistributedPlugin("SceneGraph", remote=False, ns=globals())

EXAMPLES = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "Examples", "Interactors"
)


def load_style(filename):
    """Import an example style file the way the style proxy does: by contents."""
    spec = importlib.util.spec_from_file_location(
        os.path.splitext(filename)[0], os.path.join(EXAMPLES, filename)
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class FakeStyleProxy:
    """Mocks what the style sees as `vtkSelf`, the roles it declares, and the view's
    navigation matrix."""

    def __init__(self):
        self.trackers = []
        self.buttons = []
        self.navigation = None

    def ClearAllRoles(self):
        self.trackers = []
        self.buttons = []

    def AddTrackerRole(self, role):
        self.trackers.append(role)

    def AddButtonRole(self, role):
        self.buttons.append(role)

    def AddValuatorRole(self, role):
        pass

    def GetNavigationMatrix(self):
        return self.navigation


def translation(x, y, z):
    matrix = vtkMatrix4x4()
    matrix.SetElement(0, 3, x)
    matrix.SetElement(1, 3, y)
    matrix.SetElement(2, 3, z)
    return matrix


def rotation_z(degrees):
    transform = vtkTransform()
    transform.RotateZ(degrees)
    matrix = vtkMatrix4x4()
    matrix.DeepCopy(transform.GetMatrix())
    return matrix


def scale_uniform(s):
    matrix = vtkMatrix4x4()
    for i in range(3):
        matrix.SetElement(i, i, s)
    return matrix


def as_list(matrix):
    return [matrix.GetElement(i, j) for i in range(4) for j in range(4)]


def user_transform_of(graph, node, view):
    """The node's bound representation's UserTransform, as 16 row-major values."""
    rep = graph.GetDrivenRepresentations(node, view.SMProxy)[0]
    prop = rep.GetProperty("UserTransform")
    return [prop.GetElement(i) for i in range(16)]


def origin_of(graph, node, view):
    """Where the node's bound representation has been put, from its UserTransform."""
    values = user_transform_of(graph, node, view)
    return tuple(values[i] for i in (3, 7, 11))


def close(actual, expected, tol=1e-9):
    return all(abs(a - e) <= tol for a, e in zip(actual, expected))


def scene():
    """World > Room > Bed > Pillow, plus a Wand under World, each with a sphere."""
    view = CreateRenderView()
    graph = sg.create("Interactors")
    root = graph.GetRoot()
    room = graph.CreateChild(root, "Room")
    bed = graph.CreateChild(room, "Bed")
    pillow = graph.CreateChild(bed, "Pillow")
    wand = graph.CreateChild(root, "Wand")
    graph.SetLocalTransform(room, translation(1, 0, 0))
    graph.SetLocalTransform(bed, translation(0, 1, 0))
    graph.SetLocalTransform(pillow, translation(0, 0, 1))
    for name, node in (("Room", room), ("Bed", bed), ("Pillow", pillow), ("Wand", wand)):
        source = Sphere(registrationName=name)
        Show(source, view)
        graph.AddSource(node, source.SMProxy)
    graph.TraverseAndApply()
    return view, graph, dict(room=room, bed=bed, pillow=pillow, wand=wand)


def teardown(graph):
    """The style picks the first registered graph, so each check removes its own."""
    sg.delete(graph)


def frame(style, proxy, graph):
    """Simulate an iteration of the CAVE event loop, graph is applied after Update()"""
    style.Update(proxy)
    graph.TraverseAndApply()


def wand(style, proxy, x, y, z):
    style.HandleTracker(proxy, "Wand", 0, as_list(translation(x, y, z)))


def check_grab():
    view, graph, nodes = scene()
    module = load_style("SceneGraphGrabNode.py")
    style = module.create_interactor_style()
    proxy = FakeStyleProxy()
    style.Initialize(proxy)
    assert proxy.trackers == ["Wand"] and proxy.buttons == ["Grab", "Next", "Reset"]

    # The root is first in tree order. Grab it and move the wand by (0, 0, 5)
    style.HandleButton(proxy, "Grab", 0, 1)
    wand(style, proxy, 0, 0, 0)
    frame(style, proxy, graph)  # the first frame after a press anchors the grab
    wand(style, proxy, 0, 0, 5)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (1, 0, 5))
    assert close(origin_of(graph, nodes["pillow"], view), (1, 1, 6))
    assert close(origin_of(graph, nodes["wand"], view), (0, 0, 5))
    style.HandleButton(proxy, "Grab", 0, 0)
    style.HandleButton(proxy, "Reset", 0, 1)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["wand"], view), (0, 0, 0))

    # Now grab the room, move the wand by (2, 0, 0)
    style.HandleButton(proxy, "Next", 0, 1)
    style.HandleButton(proxy, "Grab", 0, 1)
    wand(style, proxy, 10, 10, 10)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (1, 0, 0))  # anchored, not moved
    wand(style, proxy, 12, 10, 10)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (3, 0, 0))
    assert close(origin_of(graph, nodes["bed"], view), (3, 1, 0))
    assert close(origin_of(graph, nodes["pillow"], view), (3, 1, 1))
    assert close(origin_of(graph, nodes["wand"], view), (0, 0, 0))

    # After release, the wand moving no longer moves anything.
    style.HandleButton(proxy, "Grab", 0, 0)
    wand(style, proxy, 50, 50, 50)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (3, 0, 0))

    # Now check grabbing the bed moves the bed and the pillow, not the room.
    style.HandleButton(proxy, "Next", 0, 1)
    style.HandleButton(proxy, "Grab", 0, 1)
    wand(style, proxy, 0, 0, 0)
    frame(style, proxy, graph)
    wand(style, proxy, 0, 0, 3)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (3, 0, 0))
    assert close(origin_of(graph, nodes["bed"], view), (3, 1, 3))
    assert close(origin_of(graph, nodes["pillow"], view), (3, 1, 4))
    style.HandleButton(proxy, "Grab", 0, 0)

    # Simulate a few more "Next" clicks and validate target each time
    style.HandleButton(proxy, "Next", 0, 1)
    assert graph.GetNodeName(style.target) == "Pillow"
    style.HandleButton(proxy, "Next", 0, 1)
    assert graph.GetNodeName(style.target) == "Wand"
    style.HandleButton(proxy, "Next", 0, 1)
    assert graph.GetNodeName(style.target) == "World"
    style.HandleButton(proxy, "Next", 0, 1)
    assert graph.GetNodeName(style.target) == "Room"

    # Under navigation the wand's delta is conjugated into model space. With
    # a 90-degree turn about Z, a physical delta of (2, 0, 0) is a model
    # delta of (0, -2, 0) for each node.
    proxy.navigation = rotation_z(90)
    style.HandleButton(proxy, "Grab", 0, 1)
    wand(style, proxy, 0, 0, 0)
    frame(style, proxy, graph)
    wand(style, proxy, 2, 0, 0)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (3, -2, 0))
    assert close(origin_of(graph, nodes["bed"], view), (3, -1, 3))
    assert close(origin_of(graph, nodes["pillow"], view), (3, -1, 4))
    style.HandleButton(proxy, "Grab", 0, 0)

    # Reset returns every node to where it stood when the style first saw the
    # graph, including one moved from outside the style since then.
    proxy.navigation = None
    graph.SetLocalTransform(nodes["wand"], translation(9, 9, 9))
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["wand"], view), (9, 9, 9))
    style.HandleButton(proxy, "Reset", 0, 1)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (1, 0, 0))
    assert close(origin_of(graph, nodes["bed"], view), (1, 1, 0))
    assert close(origin_of(graph, nodes["pillow"], view), (1, 1, 1))
    assert close(origin_of(graph, nodes["wand"], view), (0, 0, 0))

    # In this scaled-down scene (navigation scale 0.01), a physical delta of
    # (2, 0, 0) is 200 model units, and the node's own scale is untouched.
    proxy.navigation = scale_uniform(0.01)
    style.HandleButton(proxy, "Grab", 0, 1)
    wand(style, proxy, 0, 0, 0)
    frame(style, proxy, graph)
    wand(style, proxy, 2, 0, 0)
    frame(style, proxy, graph)
    values = user_transform_of(graph, nodes["room"], view)
    assert close((values[3], values[7], values[11]), (201, 0, 0)), values
    assert close((values[0], values[5], values[10]), (1, 1, 1)), values
    assert close(origin_of(graph, nodes["pillow"], view), (201, 1, 1))
    style.HandleButton(proxy, "Grab", 0, 0)

    # Navigation read per event: a fly during a grab changes where the same
    # physical delta lands, without a re-grab.
    proxy.navigation = None
    style.HandleButton(proxy, "Reset", 0, 1)
    style.HandleButton(proxy, "Grab", 0, 1)
    wand(style, proxy, 0, 0, 0)
    frame(style, proxy, graph)
    wand(style, proxy, 1, 0, 0)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (2, 0, 0))
    proxy.navigation = rotation_z(90)
    wand(style, proxy, 2, 0, 0)
    frame(style, proxy, graph)
    assert close(origin_of(graph, nodes["room"], view), (1, -2, 0))
    style.HandleButton(proxy, "Grab", 0, 0)

    # A node added after the style loaded is baselined where the next "Next"
    # finds it, so Reset leaves it there.
    lamp = graph.CreateChild(graph.GetRoot(), "Lamp")
    graph.SetLocalTransform(lamp, translation(0, 0, 7))
    style.HandleButton(proxy, "Next", 0, 1)
    graph.SetLocalTransform(lamp, translation(0, 0, 8))
    style.HandleButton(proxy, "Reset", 0, 1)
    assert graph.GetLocalTransform(lamp).GetElement(2, 3) == 7.0, "the lamp's baseline"

    teardown(graph)
    print("grab: OK")


check_grab()
print("TestSceneGraphInteractors: OK")
