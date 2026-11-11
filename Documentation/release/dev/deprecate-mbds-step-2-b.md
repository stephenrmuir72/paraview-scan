## Output vtkPartitionedDataSetCollection instead of vtkMultiBlockDataSet in ParaView

These now output a `vtkPartitionedDataSetCollection` where they output a `vtkMultiBlockDataSet`:

| Class | Change |
| --- | --- |
| **Hierarchical Fractal** source (`vtkHierarchicalFractal`) | With **Rectilinear Grids** on, one entry per grid, grouped by level under the `LevelN` nodes of its assembly |
| **Clip** filter (`vtkPVClipDataSet`) | On an AMR input, a collection with the structure of the AMR |
| **Iso Volume** filter (`vtkIsoVolume`) | On an AMR input, a collection with the structure of the AMR |
| **Slice** filter (`vtkPVCutter`) | On an AMR input, a collection with the structure of the AMR |
