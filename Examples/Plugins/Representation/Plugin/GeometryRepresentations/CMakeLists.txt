set(classes
  vtkMySpecialPolyDataMapper
  vtkMySpecialRepresentation)

vtk_module_add_module(GeometryRepresentations
  FORCE_STATIC                      
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Representation.xml)
