from paraview.simple import *
from paraview import servermanager
from vtkmodules.vtkFiltersCore import vtkAppendFilter
from vtkmodules.vtkFiltersParallel import vtkMultiVariateToRGBA
from vtkmodules.util.numpy_support import vtk_to_numpy
import numpy as np

# Tests MultiVariateToRGBA on distributed data: the 'rgba' and 'gbc' arrays
# computed on several ranks match the ones computed serially on all the
# data, as the ranges used to normalize the components are reduced across
# the ranks. Run with pvbatch on 2 or more ranks, not in symmetric mode.

TOLERANCE = 1e-6
WAVELET_POINTS = 21 ** 3


def check(condition, what):
    if not condition:
        raise RuntimeError(what)


def fetch(source):
    """Returns the piece of each rank."""
    # without a rank, Fetch returns only the first piece of an image
    source.UpdatePipeline()
    return [servermanager.Fetch(source, rank) for rank in range(ranks)]


def by_point(pieces, names):
    """Returns {rounded point coordinates: [array values]} for the arrays.

    The values of all the arrays at a point are concatenated. Keying on the
    coordinates compares datasets that store the points in different orders,
    and merges the points shared by neighboring pieces, which have the same
    values.
    """
    values = {}
    for piece in pieces:
        arrays = [vtk_to_numpy(piece.GetPointData().GetArray(name)).reshape(
            piece.GetNumberOfPoints(), -1) for name in names]
        for i in range(piece.GetNumberOfPoints()):
            point = tuple(np.round(piece.GetPoint(i), 6))
            values[point] = np.hstack([array[i] for array in arrays])
    return values


# A Wavelet, distributed across the ranks, with a 4-component 'mv' array.
# RTData is symmetric about the center, so each rank would have the same
# ranges: the Elevation along the diagonal makes them different.
wavelet = Wavelet()
elevation = Elevation(Input=wavelet)
elevation.LowPoint = [-10, -10, -10]
elevation.HighPoint = [10, 10, 10]
calc = PythonCalculator(Input=elevation)
# Components that differ: RTData, its reverse, and two nonlinear ones. The
# Elevation, in [0, 1] along the diagonal, shifts each component by a
# different amount, so each rank's ranges differ from the global ones.
calc.Expression = ("numpy.column_stack((RTData + 200 * Elevation, 300 - RTData + 100 * Elevation, "
                   "150 + 100 * sin(RTData / 20) + 60 * Elevation, "
                   "150 + 100 * cos(RTData / 20) - 80 * Elevation))")
calc.ArrayName = "mv"

ranks = servermanager.ActiveConnection.GetNumberOfDataPartitions()
check(ranks > 1, "the test runs on more than one rank: %d" % ranks)
processIds = ProcessIds(Input=calc)
processIds.UpdatePipeline()
pidRange = processIds.PointData["PointProcessIds"].GetRange()
check(pidRange == (0, ranks - 1), "the data is distributed: process ids %s" % (pidRange,))

pieces = fetch(calc)
check(len(by_point(pieces, ["mv"])) == WAVELET_POINTS, "all the points are fetched")
append = vtkAppendFilter()
for piece in pieces:
    append.AddInputData(piece)
append.Update()
whole = append.GetOutput()

mv = MultiVariateToRGBA(Input=calc)
mv.MultiVariateArray = ["POINTS", "mv"]
mv.StoreGBC = 1


def check_matches_serial(what, partition=None, visible=None):
    serial = vtkMultiVariateToRGBA()
    serial.SetController(None)
    serial.SetInputData(whole)
    serial.SetInputArrayToProcess(0, 0, 0, 0, "mv")
    serial.SetStoreGBC(True)
    if partition:
        mv.PartitionIndices, mv.PartitionOffsets = partition
        for index in partition[0]:
            serial.AddPartitionIndex(index)
        for offset in partition[1]:
            serial.AddPartitionOffset(offset)
    if visible:
        mv.VisibleComponents = visible
        serial.SetVisibleVariableIndices(visible)
    serial.Update()

    names = ["rgba", "gbc"]
    expected = by_point([serial.GetOutput()], names)
    actual = by_point(fetch(mv), names)
    check(set(actual) == set(expected), "%s: same points" % what)
    for point, values in actual.items():
        if not np.allclose(values, expected[point], atol=TOLERANCE):
            raise RuntimeError("%s: rgba, gbc at %s: %s, expected %s"
                               % (what, point, values, expected[point]))

    mv.PartitionIndices = []
    mv.PartitionOffsets = []
    mv.VisibleComponents = []


check_matches_serial("one group")
check_matches_serial("two groups", partition=([0, 1, 2, 3], [2, 4]))
check_matches_serial("visible components", visible=[0, 2, 3])
