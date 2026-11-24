from paraview.simple import *
from paraview import servermanager, smtrace
from vtkmodules.util.numpy_support import vtk_to_numpy
import numpy as np

# Tests the Python trace of MultiVariateToRGBA: replaying the trace of a
# filter with every property changed from its default gives the same
# properties and the same output arrays.

TOLERANCE = 1e-6
PROPERTIES = {
    "MultiVariateArray": ["POINTS", "mv"],
    # 3 visible components, in 2 groups with a custom range each
    "VisibleComponents": [0, 1, 3],
    "PartitionIndices": [2, 0, 1],
    "PartitionOffsets": [1, 3],
    "CustomRange": [50, 250, 0, 300],
    "LensCenter": [0.2, -0.1],
    "LensRadius": 0.4,
    "InvertLens": 1,
    "VerticesRotation": 30.0,
    "VerticesClockwiseOrdering": 0,
    "StoreGBC": 1,
    "StoreVertices": 1,
}


def check(condition, what):
    if not condition:
        raise RuntimeError(what)


def create_pipeline():
    # a Python trace records the properties set when a proxy is created
    wavelet = Wavelet()
    calc = PythonCalculator(Input=wavelet, ArrayName="mv",
        Expression="numpy.column_stack((RTData, 300 - RTData, 150 + 100 * sin(RTData / 20), "
                   "150 + 100 * cos(RTData / 20)))")
    return MultiVariateToRGBA(Input=calc, **PROPERTIES)


def property_values(mv):
    return {name: mv.GetPropertyValue(name) for name in PROPERTIES}


def output_arrays(mv):
    mv.UpdatePipeline()
    data = servermanager.Fetch(mv)
    return {name: vtk_to_numpy(attributes.GetArray(name)).copy()
            for attributes, name in [(data.GetPointData(), "rgba"),
                                     (data.GetPointData(), "gbc"),
                                     (data.GetFieldData(), "vertices")]}


def same_values(actual, expected):
    if isinstance(expected, (list, tuple)):
        return len(actual) == len(expected) and all(
            same_values(a, e) for a, e in zip(actual, expected))
    if isinstance(expected, float):
        return abs(actual - expected) < TOLERANCE
    return actual == expected


smtrace.start_trace()
mv = create_pipeline()
trace = smtrace.stop_trace()
expectedProperties = property_values(mv)
expectedArrays = output_arrays(mv)
for name, value in PROPERTIES.items():
    check(same_values(expectedProperties[name], value), "%s set: %s" % (name, expectedProperties[name]))

ResetSession()
exec(trace, {})
replayed = FindSource("MultiVariateToRGBA1")
check(replayed is not None, "the trace creates MultiVariateToRGBA1")
actualProperties = property_values(replayed)
for name, value in expectedProperties.items():
    check(same_values(actualProperties[name], value),
          "%s in the trace: %s, expected %s\n%s" % (name, actualProperties[name], value, trace))
actualArrays = output_arrays(replayed)
for name, value in expectedArrays.items():
    check(actualArrays[name].shape == value.shape and np.allclose(actualArrays[name], value, atol=TOLERANCE),
          "%s after replaying the trace" % name)
