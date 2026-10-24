set(classes
  vtkSMSceneGraphNodeProxy
  vtkSMSceneGraphProxy)

vtk_module_add_module(ParaViewIncubator::SceneGraph
  CLASSES ${classes})
