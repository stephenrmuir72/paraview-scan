from paraview.simple import *

def get_num_ranks():
    return servermanager.vtkProcessModule.GetProcessModule().GetNumberOfLocalPartitions()

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="grid")
ProcessIdScalars()
display = Show()
view = GetActiveView()

ColorBy(display, ('POINTS', 'ProcessId'))
lut = GetColorTransferFunction('ProcessId')
lut.ApplyPreset('Rainbow Desaturated', True)
lut.RescaleTransferFunction(0, get_num_ranks())
display.SetScalarBarVisibility(view, True)

def catalyst_execute(info):
    global producer, view
    # currently, this is necessary otherwise we get deadlocks.
    view.MarkModified(None)
    ResetCamera()
    Render()
    SaveScreenshot("/tmp/image-%d.png" % info.cycle)
