from paraview.simple import *

# Tests that turning off MapScalars rescales the transfer functions for direct
# colors with opacity. Opacity is already stored in the last component, so the
# opacity function gets the full range of valid opacities: [0, 1] for floating
# point, [0, 65535] for unsigned short, or [0, 255] for the other integer
# types. With 2 components, a volume maps color from the first component; with
# 4 components, or 2 on a surface, the color map is not used and keeps its
# range, as other representations may map the same array. The selected component (e.g.
# 'Magnitude') should not matter. Python does not rescale when these
# properties change, so the test rescales as the GUI does.

# Image data with 4-component 'rgba' (double) and 'rgba8' (unsigned
# char) arrays and a 2-component 'rg' array. RGB magnitude range is in
# [0.5, 1] and alpha range in [0, 0.4], so the magnitude range does not
# overlap the alpha range.
image = ProgrammableSource(OutputDataSetType="vtkImageData")
image.Script = """
import numpy as np
# 'output' is a dataset_adapter wrapper, use the vtkImageData directly.
data = output.VTKObject
dims = (8, 8, 8)
data.dimensions = dims
t = np.linspace(0.0, 1.0, np.prod(dims))
r = 0.5 + 0.5 * t
rgba = np.column_stack((r, np.full_like(t, 0.75), np.ones_like(t), 0.4 * t))
data.point_data["rgba"] = rgba
data.point_data["rgba8"] = (255 * rgba).astype(np.uint8)
data.point_data["rg"] = rgba[:, [0, 3]].copy()
"""
image.ScriptRequestInformation = """
executive = self.GetExecutive()
executive.GetOutputInformation(0).Set(executive.WHOLE_EXTENT(), 0, 7, 0, 7, 0, 7)
"""

view = CreateRenderView()


def component_range(image, name, component):
    image.UpdatePipeline()
    return image.PointData[name].GetRange(component)


def assert_range(actual, expected, what):
    if abs(actual[0] - expected[0]) > 1e-6 or abs(actual[1] - expected[1]) > 1e-6:
        raise RuntimeError("%s range %s, expected %s" % (what, actual, expected))

# Computes the range of the transfer function
# Uses the two types of transfer functions: opacity function and color map
# Opacity function (pwf.Points): 4 values per point, x, opacity, midpoint, sharpness.
# Color map (lut.RGBPoints): 4 values per point, x, r, g, b.
def tf_range(points):
    xs = points[0::4]
    return (xs[0], xs[-1])

def set_and_rescale(display, name, value):
    setattr(display, name, value)
    display.RescaleTransferFunctionToDataRange(False, False)  # extend, force

def check(image, name, numComponents, opacityRange=(0.0, 1.0)):
    display = Show(image, view)
    ColorBy(display, ("POINTS", name, "Magnitude"))
    display.RescaleTransferFunctionToDataRange(True, False)  # extend, force
    lut = GetColorTransferFunction(name)
    pwf = GetOpacityTransferFunction(name)
    magnitudeRange = component_range(image, name, -1)
    assert_range(tf_range(pwf.Points), magnitudeRange,
        "%s opacity (MapScalars on)" % name)

    set_and_rescale(display, "MapScalars", 0)
    display.SetRepresentationType("Volume")
    assert_range(tf_range(pwf.Points), opacityRange, "%s opacity (MapScalars off)" % name)
    colorRange = component_range(image, name, 0) if numComponents == 2 else magnitudeRange
    assert_range(tf_range(lut.RGBPoints), colorRange, "%s color (MapScalars off)" % name)

    set_and_rescale(display, "MapScalars", 1)
    assert_range(tf_range(pwf.Points), magnitudeRange, "%s opacity (MapScalars on again)" % name)

    if numComponents == 2:
        # With MultiComponentsMapping, color is mapped from the first component
        # and opacity from the second.
        set_and_rescale(display, "MultiComponentsMapping", 1)
        assert_range(tf_range(lut.RGBPoints), component_range(image, name, 0),
            "%s color (MultiComponentsMapping on)" % name)
        assert_range(tf_range(pwf.Points), component_range(image, name, 1),
            "%s opacity (MultiComponentsMapping on)" % name)
        set_and_rescale(display, "MultiComponentsMapping", 0)
        assert_range(tf_range(pwf.Points), magnitudeRange,
            "%s opacity (MultiComponentsMapping off)" % name)

        # A surface uses the first component directly as luminance.
        display.SetRepresentationType("Surface")
        set_and_rescale(display, "MapScalars", 0)
        assert_range(tf_range(lut.RGBPoints), magnitudeRange,
            "%s surface color (MapScalars off)" % name)
        set_and_rescale(display, "MapScalars", 1)
    Hide(image, view)


check(image, "rgba", 4)
check(image, "rgba8", 4, opacityRange=(0.0, 255.0))
check(image, "rg", 2)
