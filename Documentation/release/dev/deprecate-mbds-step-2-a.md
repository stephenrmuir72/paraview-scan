## Handle both vtkPartitionedDataSetCollection and vtkMultiBlockDataSet in ParaView

These handled only a `vtkMultiBlockDataSet` and now handle a `vtkPartitionedDataSetCollection` too:

| Class | Change |
| --- | --- |
| **Block Ids** filter (`vtkBlockIdScalars`) | Accepts any composite input |
| **StreamLines**, **Streaming Particles** and glyph representations | Accept a `vtkPartitionedDataSetCollection` input |
| DSP plugin filters (**Mean Power Spectral Density**, **Merge Reduce Tables**, **Project Spectrum Magnitude** and **Sound Quantities Calculator**) | Accept a `vtkPartitionedDataSetCollection` input |
| Statistics filters | The model input accepts a `vtkPartitionedDataSetCollection` |
| `vtkPANLHaloFinder` and `vtkPANLSubhaloFinder` | Accept a `vtkPartitionedDataSetCollection` input; the **ANL Halo Finder** and **ANL Subhalo Finder** filters still only list an Unstructured Grid input |
| Bag plot representations | Find their blocks by name |
| `vtkMPIMoveData` | Skips the empty `vtkPartitionedDataSetCollection` pieces of ranks without data, and unsets the global ids of all composite pieces |
| `vtkOrderedCompositeDistributor` | Accepts a `vtkPartitionedDataSetCollection`, and no longer gives an empty output for a composite dataset whose leaves are not all polygonal data |
| `vtkCPAdaptorAPI` (legacy Catalyst) | Clears the field data of any composite grid |
| `vtkCPXMLPWriterPipeline` (legacy Catalyst) | Writes a `vtkPartitionedDataSetCollection` grid as a `.vtpc` file |
| **Information** panel data list (`pqDataInformationModel`) | Shows the composite icon for partitioned datasets |

`vtkPVClipDataSet::UseAMRDualClipForAMR` has had no effect since ParaView 5.11 and is deprecated.

`vtkCPXMLPWriterPipeline` now writes poly data grids with the `.pvtp` extension instead of `.pvts`.
