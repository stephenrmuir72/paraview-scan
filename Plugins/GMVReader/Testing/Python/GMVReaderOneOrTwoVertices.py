#### Verify that Merge Blocks produces polydata output when inputs are polydata
from paraview.simple import *
from paraview import servermanager as sm
from paraview import smtesting
import os;

# This test makes sure that the GMV reader can read vertices and lines.

smtesting.ProcessCommandLineArguments()

GMVDir = os.path.join(smtesting.DataDir, "Plugins", "GMVReader", "Testing", "Data", "GMV")

# load plugin
LoadDistributedPlugin('GMVReader', ns=globals())


def GetBlock(data, index):
    """Returns the block/partition at the given index of a fetched composite
    dataset. The actual composite type servermanager.Fetch() brings back
    depends on the AutoConvertMultiBlockDataSets general setting: a
    vtkMultiBlockDataSet when it is off (the default), or a
    vtkPartitionedDataSetCollection when it is on."""
    if data.IsA("vtkPartitionedDataSetCollection"):
        return data.GetPartition(index, 0)
    return data.GetBlock(index)


# create a new 'GMV Reader'
one_vertexgmv = GMVReader(registrationName='one_vertex.gmv',
        FileNames=[os.path.join(GMVDir, "one_vertex.gmv")])
one_vertexgmv.CellArrayStatus = ['material id']

assert (GetBlock(sm.Fetch(one_vertexgmv), 1).GetNumberOfVerts() == 3)

# create a new 'GMV Reader'
two_vertexgmv = GMVReader(registrationName='two_vertex.gmv',
        FileNames=[os.path.join(GMVDir, "two_vertex.gmv")])
two_vertexgmv.CellArrayStatus = ['material id']

assert (GetBlock(sm.Fetch(two_vertexgmv), 1).GetNumberOfLines() == 2)
