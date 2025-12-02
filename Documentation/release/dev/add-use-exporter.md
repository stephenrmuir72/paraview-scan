## USD exporter

A USD scene exporter is now available in ParaView. It exports the following:

* Surface polygonal geometry (verts and lines are not supported in USD) including point normals if available, cell normals otherwise.
* Color mapped surfaces through export of color map textures and texture coordinates on surfaces.
* Directional lights.
* The current camera, including position and direction. Field of view is hard-coded for now.
* Actor transforms are exported.
* Solid colored surfaces are exported, including physically-based rendering (PBR) properties supported by the USD format when PBR is enabled.
* 3D widgets that are subclasses of `vtkWidgetRepresentation` are not exported.
* All surfaces from composite poly data are exported.
