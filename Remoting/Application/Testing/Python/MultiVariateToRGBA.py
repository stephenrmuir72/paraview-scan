from paraview.simple import *
from paraview import smtesting
from vtkmodules.util.numpy_support import vtk_to_numpy
import numpy as np

# Tests the MultiVariateToRGBA filter proxy: the stored arrays, the vertices
# ordering and rotation, the lens, the partition, the custom range and the
# array selection. Then renders a slice colored by the computed RGBA.

smtesting.ProcessCommandLineArguments()

# Points are on a line. 'mv' has the zero tuple, one tuple for each
# component (k) set to 1, and the all ones tuple. 'mv2' has a last component
# 10 times larger than the others, so that the partition matters. 'mv3' has
# 3 components.
source = ProgrammableSource(OutputDataSetType="vtkImageData")
source.Script = """
import numpy as np
data = output.VTKObject
data.SetDimensions(6, 1, 1)
# mv = [[0, 0, 0, 0],
#       [1, 0, 0, 0],
#       [0, 1, 0, 0],
#       [0, 0, 1, 0],
#       [0, 0, 0, 1],
#       [1, 1, 1, 1]]
mv = np.vstack((np.zeros(4), np.eye(4), np.ones(4)))
mv2 = np.array([[0, 0, 0, 0], [1, 1, 1, 10], [0.5, 0.5, 0.5, 5],
                [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0]], dtype=float)
output.PointData.append(mv, "mv")
output.PointData.append(mv2, "mv2")
output.PointData.append(mv[:, :3].copy(), "mv3")
"""
source.ScriptRequestInformation = """
executive = self.GetExecutive()
executive.GetOutputInformation(0).Set(executive.WHOLE_EXTENT(), 0, 5, 0, 0, 0, 0)
"""

TOLERANCE = 1e-6
# vertices are placed on a circle slightly smaller than the unit circle
RADIUS = 0.997


def check(condition, what):
    if not condition:
        raise RuntimeError(what)


def check_close(actual, expected, what):
    actual = np.asarray(actual)
    if actual.shape != np.shape(expected) or not np.allclose(actual, expected, atol=TOLERANCE):
        raise RuntimeError("%s: %s, expected %s" % (what, actual, expected))


def fetch(filter):
    filter.UpdatePipeline()
    return servermanager.Fetch(filter)


def point_array(data, name):
    array = data.GetPointData().GetArray(name)
    return None if array is None else vtk_to_numpy(array)


def field_array(data, name):
    array = data.GetFieldData().GetArray(name)
    return None if array is None else vtk_to_numpy(array)


mv = MultiVariateToRGBA(Input=source)
mv.MultiVariateArray = ["POINTS", "mv"]

# Defaults: only 'rgba' is stored, and alpha is the mean of the normalized
# components. The lens covers the whole circle.
data = fetch(mv)
rgba = point_array(data, "rgba")
check(rgba is not None and rgba.shape == (6, 4), "rgba array with 4 components")
check(point_array(data, "gbc") is None, "no gbc array by default")
check(field_array(data, "vertices") is None, "no vertices array by default")
# 'mv' is in [0, 1] so normalization does not change it: alpha is the mean of
# each row, 1/4 for the rows with one component set.
check_close(rgba[:, 3], [0, 0.25, 0.25, 0.25, 0.25, 1], "alpha")
# the first component is red, the third (opposite) is cyan
check(rgba[1, 0] > 0.9 and rgba[1, 1] < 0.2 and rgba[1, 2] < 0.2, "first component is red")
check(rgba[3, 0] < 0.2 and rgba[3, 1] > 0.9 and rgba[3, 2] > 0.9, "third component is cyan")

# The first vertex is at the top, the others follow clockwise. A tuple with
# only component k set is at vertex k.
mv.StoreGBC = 1
mv.StoreVertices = 1
# vertex k is at angle 90 - k * 360 / 4 degrees: top, right, bottom, left
clockwise = RADIUS * np.array([[0, 1], [1, 0], [0, -1], [-1, 0]])
data = fetch(mv)
check_close(field_array(data, "vertices"), clockwise, "clockwise vertices")
gbc = point_array(data, "gbc")
check_close(gbc[1:5], clockwise, "gbc of single component tuples")
check_close(gbc[5], [0, 0], "gbc of the all ones tuple")

mv.VerticesClockwiseOrdering = 0
data = fetch(mv)
counterClockwise = clockwise * [-1, 1]
check_close(field_array(data, "vertices"), counterClockwise, "counter-clockwise vertices")
check_close(point_array(data, "gbc")[1:5], counterClockwise,
    "counter-clockwise gbc of single component tuples")
mv.VerticesClockwiseOrdering = 1

# VerticesRotation rotates the vertices and the gbc counter-clockwise
mv.VerticesRotation = 90
data = fetch(mv)
rotated = np.column_stack((-clockwise[:, 1], clockwise[:, 0]))
check_close(field_array(data, "vertices"), rotated, "rotated vertices")
check_close(point_array(data, "gbc")[1:5], rotated, "rotated gbc")
mv.VerticesRotation = 0

# Points outside the lens are transparent. Only the second point, on the
# first vertex, is inside.
mv.LensCenter = [0, 0.9]
mv.LensRadius = 0.3
data = fetch(mv)
check_close(point_array(data, "rgba")[:, 3], [0, 0.25, 0, 0, 0, 0], "alpha with a lens")
# With InvertLens, the points inside the lens are transparent instead.
mv.InvertLens = 1
data = fetch(mv)
check_close(point_array(data, "rgba")[:, 3], [0, 0, 0.25, 0.25, 0.25, 1],
    "alpha with an inverted lens")
mv.InvertLens = 0
mv.LensCenter = [0, 0]
mv.LensRadius = 1

# Without RGBA, only the gbc is stored
mv.StoreRGBA = 0
data = fetch(mv)
check(point_array(data, "rgba") is None, "no rgba array when StoreRGBA is off")
check(point_array(data, "gbc") is not None, "gbc array when StoreRGBA is off")
mv.StoreRGBA = 1

# With 3 components there are 3 vertices
mv.MultiVariateArray = ["POINTS", "mv3"]
data = fetch(mv)
vertices = field_array(data, "vertices")
check(vertices.shape == (3, 2), "3 vertices for 3 components")
check_close(vertices[0], [0, RADIUS], "first of 3 vertices")

# Only the visible components are processed: with components 0, 1 and 3, the
# filter sees [c0, c1, c3] and there are 3 vertices, at 90, -30 and -150
# degrees. [0, 0, 1, 0] becomes [0, 0, 0] (gbc at the center, alpha 0), and
# [0, 0, 0, 1] becomes [0, 0, 1], at the third vertex.
mv.MultiVariateArray = ["POINTS", "mv"]
mv.VisibleComponents = [0, 1, 3]
data = fetch(mv)
angles = np.radians([90, -30, -150])
three = RADIUS * np.column_stack((np.cos(angles), np.sin(angles)))
check_close(field_array(data, "vertices"), three, "vertices of the visible components")
gbc = point_array(data, "gbc")
check_close(gbc[[1, 2, 4]], three, "gbc of the visible single component tuples")
check_close(gbc[3], [0, 0], "gbc of the hidden component tuple")
check_close(point_array(data, "rgba")[:, 3], [0, 1 / 3, 1 / 3, 0, 1 / 3, 1],
    "alpha of the visible components")
mv.VisibleComponents = []

# All components are normalized together by default: the large last
# component pulls the tuple toward its vertex. In separate groups, the
# components of the second tuple are all 1, so it is at the center.
mv.MultiVariateArray = ["POINTS", "mv2"]
data = fetch(mv)
gbc = point_array(data, "gbc")
# gbc[1, 0] is the x of the second point, [1, 1, 1, 10], normalized to
# [0.1, 0.1, 0.1, 1]: the top and bottom vertices cancel, so
# x = 0.997 * (0.1 - 1) / 1.3 ~ -0.69, toward the left (last) vertex.
check(gbc[1, 0] < -0.5, "one group: tuple pulled toward the last vertex")
check_close(point_array(data, "rgba")[1, 3], 0.325, "one group: alpha")

mv.PartitionIndices = [0, 1, 2, 3]
mv.PartitionOffsets = [3, 4]
data = fetch(mv)
check_close(point_array(data, "gbc")[1], [0, 0], "two groups: tuple at the center")
check_close(point_array(data, "rgba")[1:3, 3], [1, 0.5], "two groups: alpha")

# A custom range replaces the data range of each group; values outside it
# are clamped.
mv.CustomRange = [0, 1, 0, 20]
data = fetch(mv)
check_close(point_array(data, "rgba")[1:3, 3], [0.875, 0.4375], "custom range: alpha")
mv.CustomRange = [0, 0.5, 0, 5]
data = fetch(mv)
check_close(point_array(data, "rgba")[1:3, 3], [1, 1], "clamped custom range: alpha")
mv.CustomRange = []
mv.PartitionIndices = []
mv.PartitionOffsets = []
data = fetch(mv)
check_close(point_array(data, "rgba")[1, 3], 0.325, "partition reset: alpha")

# Image: 4 gaussian blobs, one for each component, colored by the
# computed RGBA on a slice. The slice looks faded: alpha is the mean of
# the normalized components, so it stays low where one component dominates.
blobs = ProgrammableSource(OutputDataSetType="vtkImageData")
blobs.Script = """
import numpy as np
n = 32
data = output.VTKObject
data.SetDimensions(n, n, n)
data.SetSpacing(1.0 / (n - 1), 1.0 / (n - 1), 1.0 / (n - 1))
z, y, x = np.meshgrid(*(np.linspace(0, 1, n),) * 3, indexing="ij")
centers = [(0.5, 0.8), (0.8, 0.5), (0.5, 0.2), (0.2, 0.5)]
mv = np.column_stack([np.exp(-((x - cx) ** 2 + (y - cy) ** 2) / 0.08 - (z - 0.5) ** 2).ravel()
                      for cx, cy in centers])
output.PointData.append(mv, "mv")
"""
blobs.ScriptRequestInformation = """
executive = self.GetExecutive()
executive.GetOutputInformation(0).Set(executive.WHOLE_EXTENT(), 0, 31, 0, 31, 0, 31)
"""
blobsRGBA = MultiVariateToRGBA(Input=blobs)
blobsRGBA.MultiVariateArray = ["POINTS", "mv"]

view = CreateRenderView()
view.ViewSize = [400, 400]
view.OrientationAxesVisibility = 0
display = Show(blobsRGBA, view)
display.SetRepresentationType("Slice")
display.SliceMode = "XY Plane"
display.Slice = 16
ColorBy(display, ("POINTS", "rgba"))
display.MapScalars = 0
view.ResetCamera()
Render(view)

if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError("Image comparison failed.")
