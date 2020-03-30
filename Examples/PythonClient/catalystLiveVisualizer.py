from paraview.simple import *

"""
This example is intended to be used with the CxxFullExample
Particulary, it needs :
    - a pipeline source named 'Slice1'
    - a point array named 'velocity'

At each timestep, the slice is extracted and a non-interactive render view
is updated. When the connection is lost (probably due to simulation end),
the interaction is enabled in the view.

First make sure that LiveVisualization is enabled in the script.
Then:
    $ pvpython catalystLiveVisualizer.py
    $ ./CxxFullExample feslicescript.py

"""

# ----------------------------
# misc callbacks
# ----------------------------
def _updateEventCb(obj, event):
    s = FindSource('Slice1')
    global DataExtracted
    if s != None and not DataExtracted:
        extractSlice1Display = Show(s)
        DataExtracted = True
        # get color transfer function/color map for 'velocity'
        velocityLUT = GetColorTransferFunction('velocity')
        velocityLUT.AutomaticRescaleRangeMode = 'Never'
        velocityLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 325.0, 0.865003, 0.865003, 0.865003, 650.0, 0.705882, 0.0156863, 0.14902]
        velocityLUT.ScalarRangeInitialized = 1.0
        # trace defaults for the display properties.
        extractSlice1Display.Representation = 'Surface'
        extractSlice1Display.ColorArrayName = ['POINTS', 'velocity']
        extractSlice1Display.LookupTable = velocityLUT
        # init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
        extractSlice1Display.ScaleTransferFunction.Points = [0.0, 0.0, 0.5, 0.0, 136.29000000000002, 1.0, 0.5, 0.0]
        # get color legend/bar for velocityLUT in view renderView1
        velocityLUTColorBar = GetScalarBar(velocityLUT, renderView1)
        velocityLUTColorBar.Title = 'velocity'
        velocityLUTColorBar.ComponentTitle = 'Magnitude'
        # set color bar visibility
        velocityLUTColorBar.Visibility = 1
        # show color legend
        extractSlice1Display.SetScalarBarVisibility(renderView1, True)

    ResetCamera()
    Render()

def _connectionCreatedCb(obj, event):
    # name comes from the catalyst script
    ExtractCatalystData(obj, 'Slice1')
    ProcessServerNotifications()
    Render()

def _connectionClosedCb(obj, event):
    global timer
    timer.cancel()
    del timer
    print("Connection closed, interaction enabled.")
    Interact()
    print("Interaction ended.")

def _monitorServerNotifications():
    import threading
    global timer
    ProcessServerNotifications()
    timer = threading.Timer(.2, _monitorServerNotifications)
    timer.start()

# ----------------------------
# Create a render view
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [640, 480]
renderView1.AxesGrid = 'GridAxes3DActor'
renderView1.CenterOfRotation = [34.5, 32.45, 27.95]
renderView1.CameraPosition = [199.9732153733271, 32.45, 27.95]
renderView1.CameraFocalPoint = [34.5, 32.45, 27.95]
renderView1.CameraViewUp = [0.0, 0.0, 1.0]
renderView1.CameraFocalDisk = 1.0
renderView1.CameraParallelScale = 42.827619592968276

DataExtracted = False

# ----------------------------
# Create the catalyst connection
liveInsituLink = ConnectToCatalyst()
liveInsituLink.AddObserver("UpdateEvent", _updateEventCb)
liveInsituLink.AddObserver("ConnectionCreatedEvent", _connectionCreatedCb)
liveInsituLink.AddObserver("ConnectionClosedEvent", _connectionClosedCb)

# ----------------------------
# Automatically look for updates
_monitorServerNotifications()
