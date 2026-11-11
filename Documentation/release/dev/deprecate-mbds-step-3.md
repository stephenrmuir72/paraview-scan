## Write vtkPartitionedDataSetCollection in ParaView writers

These writers handled only a `vtkMultiBlockDataSet` and now handle a `vtkPartitionedDataSetCollection` too:

| Class | Change |
| --- | --- |
| **GenericIO** writer (`vtkPGenericIOMultiBlockWriter`) | Accepts a `vtkPartitionedDataSetCollection` input |
| **CGNS** writer (`vtkCGNSWriter`) | Skips partitioned datasets without partitions |

The **GenericIO** readers keep the names of the arrays they read.
