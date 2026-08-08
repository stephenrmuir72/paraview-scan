from paraview.simple import *

settings = GetSettingsProxy("GeneralSettings")

# The setting should default to off.
assert settings.AutoConvertMultiBlockDataSets == 0, (
    "Expected AutoConvertMultiBlockDataSets to default to off, got %s"
    % settings.AutoConvertMultiBlockDataSets
)

sphere = Sphere(registrationName="Sphere")
group = GroupDatasets(registrationName="Group", Input=[sphere])
group.UpdatePipeline()

dataInfo = group.GetDataInformation()
dataType = dataInfo.GetDataSetTypeAsString()
assert dataType == "vtkMultiBlockDataSet", (
    "Expected vtkMultiBlockDataSet, got %s" % dataType
)

# Turning the setting on should make newly created post filters convert
# vtkMultiBlockDataSet to vtkPartitionedDataSetCollection.
settings.AutoConvertMultiBlockDataSets = 1
try:
    sphere2 = Sphere(registrationName="Sphere2")
    group2 = GroupDatasets(registrationName="Group2", Input=[sphere2])
    group2.UpdatePipeline()

    dataInfo2 = group2.GetDataInformation()
    dataType2 = dataInfo2.GetDataSetTypeAsString()
    assert dataType2 == "vtkPartitionedDataSetCollection", (
        "Expected vtkPartitionedDataSetCollection with the setting on, got %s" % dataType2
    )
finally:
    settings.AutoConvertMultiBlockDataSets = 0

# Turning the setting back off should restore prevent conversion for newly
# created post filters.
sphere3 = Sphere(registrationName="Sphere3")
group3 = GroupDatasets(registrationName="Group3", Input=[sphere3])
group3.UpdatePipeline()

dataInfo3 = group3.GetDataInformation()
dataType3 = dataInfo3.GetDataSetTypeAsString()
assert dataType3 == "vtkMultiBlockDataSet", (
    "Expected vtkMultiBlockDataSet with the setting back off, got %s" % dataType3
)
