if (NOT TARGET ParaView::WrapRemoting)
  vtk_module_add_executable(ParaView::WrapRemoting
    vtkWrapRemoting.cxx)
endif()
