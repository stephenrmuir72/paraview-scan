set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY ${paraview_test_data_directory_output})

vtk_add_test_cxx(vtkHyperTreeGridFiltersCxxTests tests
  NO_OUTPUT NO_VALID
  TestResampleToHyperTreeGridPointArrayCxx.cxx
)

set(_vtk_build_test "HyperTreeGridFilters")
vtk_test_cxx_executable(vtkHyperTreeGridFiltersCxxTests tests)
