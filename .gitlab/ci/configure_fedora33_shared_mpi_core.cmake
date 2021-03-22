# Test VRPlugin with VRUI support
set(PARAVIEW_PLUGIN_ENABLE_VRPlugin ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_VRPlugin_USE_VRUI ON CACHE BOOL "")

# See https://gitlab.kitware.com/vtk/vtk/-/issues/17509
set(VTK_MODULE_ENABLE_ParaView_VTKExtensionsIOGeneral WANT CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora33.cmake")
