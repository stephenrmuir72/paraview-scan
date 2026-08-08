import paraview

# Compatibility version must be set before importing the simple module.
paraview.compatibility.major = 6
paraview.compatibility.minor = 2

from paraview.simple import *

settings = GetSettingsProxy("GeneralSettings")

# Enable the setting. Since the script requests compatibility with
# ParaView 6.2, newly created source proxies should still produce
# vtkMultiBlockDataSet, even though the setting says to convert to
# vtkPartitionedDataSetCollection.
settings.AutoConvertMultiBlockDataSets = 1

sphere = Sphere(registrationName="Sphere")
group = GroupDatasets(registrationName="GroupDatasets", Input=[sphere])
group.UpdatePipeline()

# Requesting compatibility with an old version should not touch the general
# setting.
assert settings.AutoConvertMultiBlockDataSets == 1, (
    "Expected AutoConvertMultiBlockDataSets to remain on, got %s"
    % settings.AutoConvertMultiBlockDataSets
)

dataInfo = group.GetDataInformation()
dataType = dataInfo.GetDataSetTypeAsString()
assert dataType == "vtkMultiBlockDataSet", (
    "Expected vtkMultiBlockDataSet for a GroupDatasets filter created while "
    "requesting compatibility with ParaView 6.2, got %s" % dataType
)
