set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY ${paraview_test_data_directory_output})

# An omega_h mesh is a directory ending in .osh, so expand the test data per
# file inside the directories, like the OpenFOAM data in VTK.
vtk_module_test_data(
  ${CMAKE_CURRENT_SOURCE_DIR}/Data/plate_6elem.osh/,REGEX:.*
  ${CMAKE_CURRENT_SOURCE_DIR}/Data/box_3d_2p.osh/,REGEX:.*
  ${CMAKE_CURRENT_SOURCE_DIR}/Data/ltx_graded_gauss.osh/,REGEX:.*
    )

if (PARAVIEW_USE_PYTHON)
  add_subdirectory(Python)
endif()

add_subdirectory(XML)
