vtk_module_third_party_internal(
  INTERFACE
  LICENSE_FILES "vtkrxcpp/license.md"
  VERSION       "master-gec38393" # post-4.1.0
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_rxcpp.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_rxcpp.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_rxcpp.h")
