set(classes
  vtkPVPythonAPI)

vtk_module_add_module(ParaView::ParaViewPythonAPI
  CLASSES ${classes})
vtk_module_client_server_exclude()
vtk_module_link(ParaView::ParaViewPythonAPI
    PRIVATE
    ParaView::paraview_plugins)
