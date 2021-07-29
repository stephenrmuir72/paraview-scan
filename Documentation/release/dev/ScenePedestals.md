## Pedestal scene elements

Two new sources have been added: the **Box Pedestal** source and the **Cylinder Pedestal** source. These new sources are intended as scene objects upon which data objects
can cast shadows when using a ray tracing backend.

The **Box Pedestal** source is essentially the same as the **Box** source, but makes use of a specific domain to initialize the pedestal, placing it just below the bounds of all data sources and filters that are visible in any view, positioning it in the center of the bounds in X and Z, setting the height to one quarter the bounds in Y, and setting the lengths in X and Z to twice the length of the corresponding side of the data bounding box.

The **Cylinder Pedestal** source is essentially the same as the **Cylinder** source and is initialized similarly to how the **Box Pedestal** is initialized. The cylinder is initialized in the center of the visible data bounds in X and Z, setting the height to one quarter the bounds in Y, and setting the radius to the maximum of the bounds length in X or Z.

Added in https://gitlab.kitware.com/paraview/paraview/-/merge_requests/5075.
