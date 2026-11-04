paraview_add_plugin(ThreeMFReader
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES ThreeMFReader::vtk3MFReader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")
