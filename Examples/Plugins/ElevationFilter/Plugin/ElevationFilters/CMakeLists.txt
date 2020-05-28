set(classes
  vtkMyElevationFilter)

vtk_module_add_module(ElevationFilters
  FORCE_STATIC                      
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  MyElevationFilter.xml)
