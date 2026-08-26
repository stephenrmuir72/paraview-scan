paraview_add_plugin(OmegaHReader
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES OmegaHReader::Reader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")
