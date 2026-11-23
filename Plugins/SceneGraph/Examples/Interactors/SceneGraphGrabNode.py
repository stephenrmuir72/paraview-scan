"""Moves a scenegraph node, and everything beneath it, using a wand.

Holding the button bound to the "Grab" role causes the target node to keep
its offset from the wand, so its whole subtree moves as the wand moves.
Releasing the button bound to the "Grab" role causes the target node to stay
where it was left.

Clicking the button bound to the "Next" role steps the target through every
node in the graph in tree order, starting with the root.

Clicking the button bound to the "Reset" role puts every node back where it
stood when the style was loaded. Any node added since then is reset to where
the first "Next" click encountered it.
"""
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from paraview.incubator import scenegraph as sg
from paraview.incubator.pythoninteractorbase import PythonInteractorBase

# Set this to name the graph to use, otherwise the script just picks one
GRAPH_NAME = None


def create_interactor_style():
    return GrabNode()


def inverse(matrix):
    result = vtkMatrix4x4()
    vtkMatrix4x4.Invert(matrix, result)
    return result


def product(*matrices):
    """The matrices multiplied left to right."""
    result = vtkMatrix4x4()
    for matrix in matrices:
        vtkMatrix4x4.Multiply4x4(result, matrix, result)
    return result


class GrabNode(PythonInteractorBase):
    def __init__(self):
        self.graph = None
        self.targets = []  # every node, in tree order, root first
        self.originals = {}  # maps every node to its local transform when first seen
        self.index = 0
        self.target = None
        self.tracker = vtkMatrix4x4()  # the latest wand pose
        self.fresh = False
        self.grabbing = False  # Whether a grab is in progress
        self.inverse_press = None  # the wand pose on button press, inverted
        self.world_at_press = None  # the target's world transform on button press
        self.inverse_parent = None  # parent's world transrom on button press, inverted


    def Initialize(self, vtkSelf):
        vtkSelf.ClearAllRoles()
        vtkSelf.AddTrackerRole("Wand")
        vtkSelf.AddButtonRole("Grab")
        vtkSelf.AddButtonRole("Next")
        vtkSelf.AddButtonRole("Reset")

    def HandleButton(self, vtkSelf, role, button, state):
        if role == "Grab":
            if state == 1:
                self._begin_grab()
            else:
                self.grabbing = False
                self.fresh = False
        elif role == "Next" and state == 1:
            self.grabbing = False
            self.fresh = False
            self._step_target()
        elif role == "Reset" and state == 1:
            self.grabbing = False
            self.fresh = False
            self._reset()

    def HandleTracker(self, vtkSelf, role, sensor, matrix):
        # Do minimal work here, all computation happens in Update()
        if role != "Wand":
            return
        self.tracker.DeepCopy(matrix)
        self.fresh = self.grabbing

    def Update(self, vtkSelf):
        if not self.fresh:
            return
        self.fresh = False
        if self.inverse_press is None:
            # The first pose after the press is the one every later pose is
            # measured against.
            self.inverse_press = inverse(self.tracker)
        navigation = vtkSelf.GetNavigationMatrix() or vtkMatrix4x4()
        delta = product(self.tracker, self.inverse_press)
        world = product(inverse(navigation), delta, navigation, self.world_at_press)
        self.graph.SetLocalTransform(self.target, product(self.inverse_parent, world))

    def _begin_grab(self):
        if self.target is None:
            if not self._resolve():
                return
            self._select(0)
        # Read the parent on button down, and assume it doesn't move during the grab
        parent = self.graph.GetParent(self.target)
        self.inverse_parent = (
            inverse(self.graph.GetWorldTransform(parent)) if parent else vtkMatrix4x4()
        )
        self.world_at_press = self.graph.GetWorldTransform(self.target)
        self.inverse_press = None
        self.grabbing = True

    def _resolve(self):
        """Read the graph's nodes in tree order, recording the placement of
        any node not seen before."""
        graph = sg.find(GRAPH_NAME) if GRAPH_NAME else next(iter(sg.all_graphs()), None)
        if graph is None:
            return False
        targets = []
        pending = [graph.GetRoot()]
        while pending:
            node = pending.pop()
            targets.append(node)
            pending.extend(reversed(graph.GetChildren(node)))
        for node in targets:
            if node not in self.originals:
                self.originals[node] = graph.GetLocalTransform(node)
        self.graph = graph
        self.targets = targets
        return True

    def _select(self, index):
        self.index = index % len(self.targets)
        self.target = self.targets[self.index]

    def _step_target(self):
        # Re-read the list each time, so nodes added or removed since are seen.
        if not self._resolve():
            self.target = None
            return
        self._select(self.index + 1)

    def _reset(self):
        # Re-read the graph first, so we don't try to update a removed node
        if not self._resolve():
            return
        for node in self.targets:
            self.graph.SetLocalTransform(node, self.originals[node])
