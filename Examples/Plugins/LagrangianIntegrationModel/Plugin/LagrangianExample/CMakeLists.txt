set(classes
  vtkLagrangianIntegrationModelExample)

vtk_module_add_module(LagrangianExample
  FORCE_STATIC                      
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  LagrangianIntegrationModel.xml)
