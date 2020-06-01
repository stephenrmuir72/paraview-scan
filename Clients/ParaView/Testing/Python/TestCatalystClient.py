from paraview.simple import *

"""
This example demonstrates how to use python to connect to a Catalyst session.

At each timestep, the slice is extracted and a non-interactive render view
is updated. When the connection is lost (probably due to simulation end),
the interaction is enabled in the view.

First make sure that LiveVisualization is enabled in the script. Then run this example.
Last run the simulation.

For instance:
    modify Examples/Catalyst/CxxFullExample/SampleScripts/feslicescript.py
    ```
    $ pvpython catalystLiveVisualizer.py
    $ ./CxxFullExample feslicescript.py
    ```
"""

# ----------------------------
# misc callbacks
# ----------------------------
def _updateEventCb(obj, event):
    global DataExtracted
    global liveInsituLink

    name = _getSourceToExtractName(obj)
    source = FindSource(name)

    if source != None and not DataExtracted:
        DataExtracted = True
        rep = Show(source)
        rep.Representation = 'Surface'
        info = source.GetPointDataInformation()
        if info.GetNumberOfArrays() > 0:
            arrName = info.GetArray(0).Name
            rep.ColorArrayName = ['POINTS', arrName]
            ColorBy(rep, arrName)

    prop = GetDisplayProperties(source)
    prop.RescaleTransferFunctionToDataRange(False, True)
    ResetCamera()
    Render()


    if source != None:
        dataObject = source.GetClientSideObject().GetOutputDataObject(0)
        currentTime = dataObject.GetFieldData().GetArray('TimeValue').GetValue(0)
        # if connection closes due to simulation end, it creates error messages
        # leading to test failure. So stop live visu before the end of simu.
        if  currentTime >= 0.5:
            liveInsituLink.RemoveObservers("UpdateEvent")
            liveInsituLink.RemoveObservers("ConnectionClosedEvent")
            EndTest()

def _connectionCreatedCb(obj, event):
    # name comes from the catalyst script
    name = _getSourceToExtractName(obj)
    ExtractCatalystData(obj, name)
    ProcessServerNotifications()
    Render()

def _connectionClosedCb(obj=None, event=None):
    print("Connection closed")
    Render()
    EndTest()


def EndTest():
    global renderView1
    # stop watching for events to process.
    global ProcessEvents
    ProcessEvents = False

    import os
    import sys
    try:
        baselineIndex = sys.argv.index('-B')+1
        baselinePath = sys.argv[baselineIndex]
    except:
        print("Could not get baseline directory. Test failed.")
    baseline_file = os.path.join(baselinePath, "TestCatalystClient.png")
    from paraview.vtk.test import Testing
    from paraview.vtk.util.misc import vtkGetTempDir
    Testing.VTK_TEMP_DIR = vtkGetTempDir()
    Testing.compareImage(renderView1.GetRenderWindow(), baseline_file)
    Testing.interact()


# ----------------------------
# Utilities methods
# ----------------------------
def _monitorServerNotifications():
    import threading
    global ProcessEvents
    ProcessEvents = True
    ProcessServerNotifications()
    if ProcessEvents:
        threading.Timer(.2, _monitorServerNotifications).start()

def _getSourceToExtractName(insituLink):
    pm = insituLink.GetInsituProxyManager()
    name = 'input'
    proxy = pm.GetProxy('sources', name)
    if proxy == None:
        # if no one exists, pick another one
        name = pm.GetProxyName('sources', pm.GetNumberOfProxies('sources') - 1)

    return name
# ----------------------------
# Create a render view
renderView1 = CreateRenderView()

DataExtracted = False

print("started")
# ----------------------------
# Create the catalyst connection
liveInsituLink = ConnectToCatalyst()
liveInsituLink.AddObserver("UpdateEvent", _updateEventCb)
liveInsituLink.AddObserver("ConnectionCreatedEvent", _connectionCreatedCb)
liveInsituLink.AddObserver("ConnectionClosedEvent", _connectionClosedCb)

# ----------------------------
# Automatically look for updates
_monitorServerNotifications()
