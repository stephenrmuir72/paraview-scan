from paraview.simple import *

# Tests that turning on UseSeparateOpacityArray with an opacity array that
# does not exist (e.g. a stale name) does not crash: the transfer functions
# are rescaled and the opacity function gets the range of the color array.

wavelet = Wavelet()
cells = PointDatatoCellData(Input=wavelet)
cells.UpdatePipeline()

view = CreateRenderView()
display = Show(cells, view)
ColorBy(display, ("CELLS", "RTData"))
display.OpacityArrayName = ["POINTS", "missing"]
display.UseSeparateOpacityArray = 1

colorRange = cells.CellData["RTData"].GetRange()
points = GetOpacityTransferFunction("RTData").Points
opacityRange = (points[0], points[-4])
if abs(opacityRange[0] - colorRange[0]) > 1e-6 or abs(opacityRange[1] - colorRange[1]) > 1e-6:
    raise RuntimeError("opacity range %s, expected %s" % (opacityRange, colorRange))
