import os
import re
import tempfile

from paraview import servermanager
from paraview.simple import *

# Checks that loading a state decides correctly if vtkMultiBlockDataSets are
# converted to vtkPartitionedDataSetCollections, no matter the setting at load time.

stateDir = tempfile.mkdtemp()


def setAutoConvert(value):
    GetSettingsProxy("GeneralSettings").AutoConvertMultiBlockDataSets = value


def saveState(name, autoConvert, withMultiBlockOnlyProxy):
    ResetSession()
    setAutoConvert(autoConvert)
    sphere = Sphere(registrationName="Sphere")
    GroupDatasets(registrationName="Group", Input=[sphere])
    if withMultiBlockOnlyProxy:
        # This writer only accepts vtkMultiBlockDataSets.
        writer = servermanager.ProxyManager().NewProxy(
            "insitu_writer_parameters", "XMLMultiBlockDataWriter"
        )
        servermanager.ProxyManager().RegisterProxy(
            "insitu_writer_parameters", "Writer", writer
        )
    path = os.path.join(stateDir, name + ".pvsm")
    SaveState(path)
    return path


def editState(path, version=None, removeAutoConvert=False):
    with open(path) as f:
        text = f.read()
    if version:
        text = re.sub(
            r'(<ServerManagerState version=")[^"]*"', r'\g<1>%s"' % version, text
        )
    if removeAutoConvert:
        text = re.sub(r' auto_convert_multiblock_datasets="\d"', "", text)
    with open(path, "w") as f:
        f.write(text)
    return path


def loadedDataType(path):
    ResetSession()
    setAutoConvert(1)
    LoadState(path)
    group = FindSource("Group")
    group.UpdatePipeline()
    return group.GetDataInformation().GetDataSetTypeAsString()


def check(description, path, expected):
    dataType = loadedDataType(path)
    assert dataType == expected, "%s: expected %s, got %s" % (
        description,
        expected,
        dataType,
    )


# Saved by a version that converts, with nothing that needs vtkMultiBlockDataSets.
current = editState(saveState("current", 1, False), version="6.3.0")
check("6.3 state", current, "vtkPartitionedDataSetCollection")

# Saved without the conversion.
notConverted = editState(saveState("notConverted", 0, False), version="6.3.0")
check("state saved without conversion", notConverted, "vtkMultiBlockDataSet")

# A proxy that only accepts vtkMultiBlockDataSets is in the state.
multiBlockOnly = editState(saveState("multiBlockOnly", 1, True), version="6.3.0")
check("state with a multiblock only proxy", multiBlockOnly, "vtkMultiBlockDataSet")

# States from before the conversion existed have no record of it.
old = editState(
    saveState("old", 1, False), version="6.2.0", removeAutoConvert=True
)
check("pre-6.3 state", old, "vtkMultiBlockDataSet")
