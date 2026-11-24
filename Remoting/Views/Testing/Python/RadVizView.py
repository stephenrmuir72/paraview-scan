from paraview.simple import *
from paraview import smtesting
from vtkmodules.vtkCommonCore import vtkCommand
import numpy as np

# Tests the RadViz view and representation: their defaults, the series
# visibility and labels passed to the plot, the properties pushed to the
# plot, and the properties updated from the plot at the end of an
# interaction. Then renders the view.

smtesting.ProcessCommandLineArguments()

# 4 gaussian blobs, one for each component of 'mv'. Points near the center
# of blob k are dominated by component k, so they are near vertex k in the
# RadViz view.
blobs = ProgrammableSource(OutputDataSetType="vtkImageData")
blobs.Script = """
import numpy as np
n = 10
data = output.VTKObject
data.SetDimensions(n, n, n)
z, y, x = np.meshgrid(*(np.linspace(0, 1, n),) * 3, indexing="ij")
centers = [(0.5, 0.8), (0.8, 0.5), (0.5, 0.2), (0.2, 0.5)]
mv = np.column_stack([np.exp(-((x - cx) ** 2 + (y - cy) ** 2) / 0.08 - (z - 0.5) ** 2).ravel()
                      for cx, cy in centers])
output.PointData.append(mv, "mv")
"""
blobs.ScriptRequestInformation = """
executive = self.GetExecutive()
executive.GetOutputInformation(0).Set(executive.WHOLE_EXTENT(), 0, 9, 0, 9, 0, 9)
"""
mv = MultiVariateToRGBA(Input=blobs)
mv.MultiVariateArray = ["POINTS", "mv"]

TOLERANCE = 1e-6
COMPONENTS = ["mv_0", "mv_1", "mv_2", "mv_3"]


def check(condition, what):
    if not condition:
        raise RuntimeError(what)


def check_close(actual, expected, what):
    actual = np.atleast_1d(np.asarray(actual, dtype=float))
    if actual.shape != np.atleast_1d(expected).shape or not np.allclose(
            actual, expected, atol=TOLERANCE):
        raise RuntimeError("%s: %s, expected %s" % (what, actual, expected))


def strings(array):
    return [array.GetValue(i) for i in range(array.GetNumberOfValues())]


# Only the RadViz view has a bold title by default, as vtkChartRadViz
lineChart = CreateView("XYChartView")
check(lineChart.ChartTitleBold == 0, "line chart title is not bold")
Delete(lineChart)
view = CreateView("RadVizChartView")
view.ViewSize = [400, 400]
check(view.ChartTitleBold == 1, "RadViz title is bold")

# Points are white with a size of 6 by default, as vtkPlotRadViz.
display = Show(blobs, view, "RadVizRepresentation")
check_close(display.Color, [1, 1, 1], "default color")
check(display.MarkerSize == 6, "default marker size")

# Set up the representation as 'Show RadViz' does: the components of the
# multivariate array are the visible series.
display.MultiVariateSource = mv
display.OutsideLensAlpha = 0.1
display.SeriesVisibility = COMPONENTS
display.ScalarVisibility = 1
# Rename the second component. The plot gets a label for every column,
# visible or not.
labels = list(display.LabelName)
labels[labels.index("mv_1") + 1] = "East"
display.LabelName = labels
Render(view)

chart = display.GetClientSideObject().GetChart()
plot = chart.GetPlot(0)
columns = [plot.GetInput().GetColumnName(i) for i in range(plot.GetInput().GetNumberOfColumns())]
check(strings(chart.GetVisibleVariables()) == COMPONENTS, "visible variables")
plotLabels = strings(plot.GetLabels())
check(len(plotLabels) == len(columns), "one label for each column")
check(plotLabels[:4] == ["mv_0", "East", "mv_2", "mv_3"], "component labels: %s" % plotLabels)

# The representation properties are pushed to the plot
display.VerticesRotation = 30
display.VerticesClockwiseOrdering = 0
display.LensCenter = [0.3, 0.2]
display.LensRadius = 0.5
Render(view)
check_close(plot.GetVerticesRotation(), 30, "plot vertices rotation")
check(not plot.GetVerticesClockwiseOrdering(), "plot vertices ordering")
check_close(plot.GetLensCenter(), [0.3, 0.2], "plot lens center")
check_close(plot.GetLensRadius(), 0.5, "plot lens radius")
check_close(plot.GetOutsideLensAlpha(), 0.1, "plot outside lens alpha")
check(plot.GetScalarVisibility(), "plot scalar visibility")

# Changes in the plot, as made by dragging the vertices or the lens, update
# the representation properties at the end of the interaction.
plot.InvokeEvent(vtkCommand.StartInteractionEvent)
plot.SetVerticesRotation(45)
plot.SetLensCenter(0.0, 0.5)
plot.SetLensRadius(0.6)
check_close(display.VerticesRotation, 30, "rotation before the end of the interaction")
plot.InvokeEvent(vtkCommand.EndInteractionEvent)
check_close(display.VerticesRotation, 45, "rotation after the interaction")
check_close(display.LensCenter, [0.0, 0.5], "lens center after the interaction")
check_close(display.LensRadius, 0.6, "lens radius after the interaction")

# The plot keeps the rotation in [0, 360): -10 is stored as 350, which is the
# same angle, so a later change of the plot is not taken for an interaction.
display.VerticesRotation = -10
Render(view)
check_close(plot.GetVerticesRotation(), 350, "plot rotation for -10")
interactions = []
observer = display.GetClientSideObject().AddObserver(
    vtkCommand.InteractionEvent, lambda caller, event: interactions.append(event))
plot.Modified()
display.GetClientSideObject().RemoveObserver(observer)
check(not interactions, "no interaction for the same angle")
check_close(display.VerticesRotation, -10, "rotation after a change of the plot")

# The representations of a view share its plot: showing another one hides
# 'display', which ignores the plot's changes, and removing the hidden one
# leaves the plot to the visible one.
other = MultiVariateToRGBA(Input=blobs)
other.MultiVariateArray = ["POINTS", "mv"]
otherDisplay = Show(other, view, "RadVizRepresentation")
otherDisplay.SeriesVisibility = COMPONENTS
Render(view)
check(display.Visibility == 0, "showing another representation hides the first")
plot.InvokeEvent(vtkCommand.StartInteractionEvent)
plot.SetVerticesRotation(60)
plot.InvokeEvent(vtkCommand.EndInteractionEvent)
check_close(otherDisplay.VerticesRotation, 60, "rotation of the visible representation")
check_close(display.VerticesRotation, -10, "rotation of the hidden representation")
Hide(other, view)
display = Show(blobs, view, "RadVizRepresentation")
Render(view)
Delete(otherDisplay)
Delete(other)
Render(view)
check(chart.GetVisible() and plot.GetInput() is not None, "plot kept after removing a hidden one")

display.VerticesRotation = 0
display.VerticesClockwiseOrdering = 1
Render(view)

if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError("Image comparison failed.")
