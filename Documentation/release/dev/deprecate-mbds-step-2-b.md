## Output vtkPartitionedDataSetCollection instead of vtkMultiBlockDataSet in ParaView

These now output a `vtkPartitionedDataSetCollection` where they output a `vtkMultiBlockDataSet`:

| Class | Change |
| --- | --- |
| **Hierarchical Fractal** source (`vtkHierarchicalFractal`) | With **Rectilinear Grids** on, one entry per grid, grouped by level under the `LevelN` nodes of its assembly |
| **Clip** filter (`vtkPVMetaClipDataSet`, `vtkPVClipDataSet`) | On an AMR input, a collection with the structure of the AMR |
| **Glyph** filter (`vtkPVGlyphFilter`) | On an AMR input, a collection with the structure of the AMR (it gave an AMR) |
| **Iso Volume** filter (`vtkIsoVolume`) | On an AMR input, a collection with the structure of the AMR |
| **Slice** filter (`vtkPVMetaSliceDataSet`, `vtkPVCutter`) | On an AMR input, a collection with the structure of the AMR |
| Catalyst steering source (`vtkSteeringDataGenerator`) | A collection with one partition under the `Block0` node of its assembly, so the Catalyst results keep their layout |
