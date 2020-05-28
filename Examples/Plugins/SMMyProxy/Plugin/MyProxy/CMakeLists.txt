set(classes
  vtkSMMyBoundsDomain
  vtkSMMyElevationProxy)

vtk_module_add_module(MyProxy
  FORCE_STATIC                      
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  MyProxy.xml)
