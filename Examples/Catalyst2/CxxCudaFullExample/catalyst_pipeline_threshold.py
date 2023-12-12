from paraview.simple import *
from paraview import catalyst
import time

# Greeting to ensure that ctest knows this script is being imported
print("executing catalyst_pipeline_threshold")

# Load VTKmFilters plugin
LoadDistributedPlugin('VTKmFilters', remote=False, ns=globals())

# registrationName must match the channel name used in the
# 'CatalystAdaptor'.
producer = TrivialProducer(registrationName="grid")
p2cell = VTKmPointDatatoCellData(registrationName='p2cell', Input=producer)
threshold = VTKmThreshold(registrationName='threshold', Input=p2cell)
threshold.Scalars = ['CELLS', 'velocity']
threshold.LowerThreshold = 0
threshold.UpperThreshold = 15

# ------------------------------------------------------------------------------
# Catalyst options
options = catalyst.Options()
if "--enable-live" in catalyst.get_args():
  options.EnableCatalystLive = 1

def catalyst_execute(info):
    global threshold
    threshold.UpdatePipeline()
    print("-----------------------------------")
    print("executing (cycle={}, time={})".format(info.cycle, info.time))
    print("bounds:", threshold.GetDataInformation().GetBounds())
    print("velocity-magnitude-range:", threshold.CellData["velocity"].GetRange(-1))
    # Prints the velocity range at source. Use this as a guide for lower/upper threshold values.
    # print("velocity-x-range:", p2cell.CellData["velocity"].GetRange(0))
    # print("velocity-y-range:", p2cell.CellData["velocity"].GetRange(1))
    # print("velocity-z-range:", p2cell.CellData["velocity"].GetRange(2))
    # In a real simulation sleep is not needed. We use it here to slow down the
    # "simulation" and make sure ParaView client can catch up with the produced
    # results instead of having all of them flashing at once.
    if options.EnableCatalystLive:
        time.sleep(1)
