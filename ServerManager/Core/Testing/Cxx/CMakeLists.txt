vtk_add_test_cxx(vtkServerManagerCoreCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  #  TestPartialArraysInformation.cxx
  # TestPVArrayInformation.cxx
  # TestSpecialDirectories.cxx
  TestProcessModule.cxx
  TestRemoteObjectProvider.cxx
  )

vtk_add_test_cxx(vtkServerManagerCoreCxxTests tests
  --server
  TestProcessModuleForServer,TestProcessModule.cxx,NO_VALID)

vtk_test_cxx_executable(vtkServerManagerCoreCxxTests tests)
