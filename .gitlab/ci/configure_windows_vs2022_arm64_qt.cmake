set(PARAVIEW_PLUGIN_ENABLE_pvNVIDIAIndeX OFF CACHE BOOL "")
set(PARAVIEW_XRInterface_OpenXR_Support OFF CACHE BOOL "")
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_windows.cmake")
