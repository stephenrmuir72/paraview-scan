## Convert Multi-block Datasets to Partitioned Dataset Collections

ParaView can now convert the Multi-block Dataset output of readers and filters to a Partitioned Dataset Collection. This
is controlled by the **Auto Convert Multi Block Data Sets** setting in the _General_ settings, under _Data Processing
Options_. When it is on, Multi-block Datasets are converted as they leave the source or filter that produced them, so
the filters downstream of it, the _Information_ panel and the data you fetch from Python all see a Partitioned Dataset
Collection.

State files and Python scripts written for ParaView 6.2 or older keep producing Multi-block Datasets. The same is true
of state files that were saved without the conversion, and of state files that contain a filter that only accepts
Multi-block Datasets. To get Multi-block Datasets in a Python script that targets a newer version, turn the setting off.

### Fixes

* The **Plot Edges** filter names its blocks on all ranks, not only on the first one.

## Developer notes

`vtkPVPostFilter` has a new `AutoConvertMultiBlockDataSets` property that converts a `vtkMultiBlockDataSet` input to a
`vtkPartitionedDataSetCollection` using `vtkConvertToPartitionedDataSetCollection`.
`vtkSMSourceProxy::SetDisableAutoConvertMultiBlockDataSets()` turns the conversion off for the post filters of a single
source proxy. Python scripts and state files saved before ParaView 6.3 use it to keep their behavior.
`ServerManagerState` elements now have an `auto_convert_multiblock_datasets` attribute.
The data information of a source proxy now comes from its post filter output, so it reports the converted type.
The legacy Catalyst `vtkCPXMLPWriterPipeline` turns the conversion off on its producer, since it picks its writer
from the grid type.
