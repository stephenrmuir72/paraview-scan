vtk_module_third_party_internal(
  INTERFACE
  LICENSE_FILES "vtkrxqt/LICENSE"
  VERSION       "master"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_rxqt.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_rxqt.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_rxqt.h")
