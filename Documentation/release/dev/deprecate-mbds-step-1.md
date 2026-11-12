## Deprecate ParaView classes for vtkPartitionedDataSetCollection

VTK is moving from `vtkMultiBlockDataSet` to `vtkPartitionedDataSetCollection`. ParaView follows
with these changes.

### Deprecations

These are deprecated and will be removed in ParaView 6.5:

| Deprecated | Replacement |
| --- | --- |
| **Convert AMR dataset to Multi-block** filter | **Convert To PartitionedDataSetCollection** |
| **Extract AMR Blocks** filter | **Extract Block**, with `/Root/LevelN` selectors to extract whole levels |
| **InSituExodus** plugin | **IOSS Reader** |
| **GmshReader** plugin (already deprecated since ParaView 5.10), `vtkGmshMetaReader` and `vtkGmshReader` | **Gmsh Meta-File Reader** (`vtkGmshMetaFileReader`) of the **GmshIO** plugin |
| **AdiosReaderPixie** plugin and `vtkAdiosPixieReader` | ADIOS2 readers |
| **VisIt OpenFOAM Reader** | **OpenFOAM** reader, which now also opens `controlDict` files |
| **InSituParticlePath** filter, `vtkInSituPParticlePathFilter`, `vtkLegacyPParticlePathFilter`, `vtkLegacyPParticleTracerBase` and `vtkLegacyParticleTracerBase` | **ParticlePath** (`vtkPParticlePathFilter`); Catalyst restart is not supported |
| `vtkCPMultiBlockGridBuilder` | None, it is unused |
| NPIC and Pagosa legacy Catalyst adaptors | None, they are unused |

### New reader

The **GmshIO** plugin has a new **Gmsh Meta-File Reader** (`vtkGmshMetaFileReader`) for the `.mshi` meta-files
that the **GmshReader** plugin reads: partitioned `.msh` files, field files per time step and adaptive refinement of
high-order fields. It outputs a `vtkPartitionedDataSetCollection` with one partition per `.msh` file. The **GmshIO**
plugin now requires Gmsh 4.12 or newer.

### Fixes

* **Python View** gathers a `vtkPartitionedDataSetCollection` from all ranks; it used the deprecated
  `vtkMultiBlockDataGroupFilter`, which refuses one.

### Developer notes

* The **GeometryFilter** proxy uses `vtkGeometryFilterDispatcher` instead of the deprecated `vtkPVGeometryFilter`; its
  properties are unchanged.
