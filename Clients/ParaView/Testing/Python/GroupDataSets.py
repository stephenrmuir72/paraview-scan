from paraview.simple import *


def GetTopLevelBlockNames(dataInfo):
    """Returns the ordered list of top-level block/partition names for a
    composite dataset. Works uniformly whether the underlying data is a
    vtkMultiBlockDataSet or a vtkPartitionedDataSetCollection, since
    it accesses the hierarchy of the dataset, which is available for
    both vtkMultiBlockDataSet and vtkPartitionedDataSetCollection."""
    hierarchy = dataInfo.GetHierarchy()
    root = hierarchy.GetRootNode()
    return [
        hierarchy.GetAttributeOrDefault(node, "label", hierarchy.GetNodeName(node))
        for node in hierarchy.GetChildNodes(root, False)
    ]


s = Sphere(registrationName="Sphere")
w = Wavelet(registrationName="Wavelet")

for outputType in ["vtkMultiBlockDataSet", "vtkPartitionedDataSetCollection"]:
    g = GroupDatasets(Input=[s, w])
    g.OutputType = outputType

    # Confirm that the defaults were initialized as expected.
    assert g.BlockNames == ["Sphere", "Wavelet"]
    g.UpdatePipeline()

    dataInfo = g.GetDataInformation()
    assert GetTopLevelBlockNames(dataInfo) == ["Sphere", "Wavelet"]

    # change block names
    g.BlockNames = ["Random"]
    g.UpdatePipeline()

    dataInfo = g.GetDataInformation()
    assert GetTopLevelBlockNames(dataInfo) == ["Random", "Block 1"]

    Delete(g)
