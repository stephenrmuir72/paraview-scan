from paraview import catalyst  # noqa: F401
from paraview.modules.vtkPVInSitu import vtkInSituInitializationHelper
from vtkmodules.vtkCommonCore import vtkIdTypeArray
from vtkmodules.vtkParallelCore import vtkCommunicator, vtkMultiProcessController

controller = vtkMultiProcessController.GetGlobalController()
rank = controller.GetLocalProcessId()
size = controller.GetNumberOfProcesses()

# Cells over all ranks, and the channel that is empty on ranks other than 0.
mesh_channels = ({"grid": 8, "amr": 8, "blocks": 4 * (size - 1)}, "grid")
fides_channels = ({"fides": 2}, "fides")


def global_sum(value):
    local = vtkIdTypeArray()
    local.InsertNextValue(value)
    total = vtkIdTypeArray()
    controller.AllReduce(local, total, vtkCommunicator.SUM_OP)
    return total.GetValue(0)


def catalyst_execute(info):
    has_fides = vtkInSituInitializationHelper.GetProducer("fides") is not None
    expected, empty = fides_channels if has_fides else mesh_channels
    for channel, cells in expected.items():
        producer = vtkInSituInitializationHelper.GetProducer(channel)
        if producer is None:
            raise RuntimeError(f"no producer for channel '{channel}'")
        producer.UpdatePipeline(info.time)
        information = producer.GetDataInformation()
        found = global_sum(information.GetNumberOfCells())
        if found != cells:
            raise RuntimeError(f"channel '{channel}' has {found} cells, expected {cells}")
        if channel == empty:
            partitions = information.GetNumberOfDataSets() if rank != 0 else 0
            if global_sum(partitions) != 0:
                raise RuntimeError(f"an empty '{channel}' should have no partitions")
