## Add a 3MF reader plugin

ParaView now provides the ThreeMFReader plugin to read 3D Manufacturing Format (`.3mf`) files, as produced by CAD tools and 3D printing slicers. The **3MF Reader** produces a Partitioned Dataset Collection with one Polygonal Mesh partition per build item, named after its object, with build and component transforms applied.

Colors defined by base materials and color groups are provided by the `Color` cell array, which you can display directly by turning off **Map Scalars**, and the source object of each triangle by the `ObjectId` cell array. The model unit is reported by the `Unit` field data array, and model metadata such as the title or designer are available as field data. Enable the **Convert To Millimeters** property to scale coordinates to millimeters whatever the model unit.

Objects stored in separate model parts of the package using the 3MF Production extension, as written by several slicers, are supported. Textures, beam lattices and slices are not read.
