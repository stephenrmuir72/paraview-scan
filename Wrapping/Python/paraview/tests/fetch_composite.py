"""
Test servermanager.Fetch with composite input
"""
import paraview
from paraview.simple import *

def main():
    sphere1 = Sphere()

    # Test vtkPartitionedDataSetCollection output
    axisAlignedReflect1 = AxisAlignedReflect(Input=sphere1)
    axisAlignedReflect1.Set(
        PlaneMode='Y Min',
    )
    axisAlignedReflect1.UpdatePipeline()

    info = paraview.servermanager.Fetch(axisAlignedReflect1)
    assert info.GetClassName() == "vtkPartitionedDataSetCollection"

    # Test vtkMultiBlockDataSet output
    groupDatasets1 = GroupDatasets(Input=sphere1)
    groupDatasets2 = GroupDatasets(Input=groupDatasets1)

    groupDatasets2.UpdatePipeline()

    info = paraview.servermanager.Fetch(groupDatasets2)

    generalSettings = GetSettingsProxy("GeneralSettings")
    pdc_output = generalSettings.AutoConvertMultiBlockDataSets

    if pdc_output:
        assert info.GetPartition(0, 0).GetClassName() == "vtkPolyData"
    else:
        assert info.GetBlock(0).GetClassName() == "vtkMultiBlockDataSet"

    groupDatasets2.CombineFirstLayerMultiblock = True
    groupDatasets2.UpdatePipeline()

    info = paraview.servermanager.Fetch(groupDatasets2)

    if pdc_output:
        assert info.GetPartition(0, 0).GetClassName() == "vtkPolyData"
    else:
        assert info.GetBlock(0).GetClassName() == "vtkPolyData"


if __name__ == "__main__":
    main()
