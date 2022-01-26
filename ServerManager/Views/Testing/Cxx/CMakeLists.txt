vtk_add_test_cxx(vtkServerManagerViewsCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestBasicRenderView.cxx)

vtk_test_cxx_executable(vtkServerManagerViewsCxxTests tests)
