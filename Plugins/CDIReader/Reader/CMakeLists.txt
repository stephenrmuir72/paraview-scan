set(classes
  vtkCDIReader)

set (sources
    cdi_tools.cxx
    projections.cxx
    )
set (private_headers
    cdi_tools.h
    projections.h)

vtk_module_add_module(CDIReader::vtkCDIReader
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers}
)

set(CMAKE_FIND_DEBUG_MODE TRUE)
vtk_module_find_package(
  PACKAGE CDI
  VERSION 2)
set(CMAKE_FIND_DEBUG_MODE FALSE)

vtk_module_link(CDIReader::vtkCDIReader
  PRIVATE
  CDI::CDI)

paraview_add_server_manager_xmls(
  XMLS  CDIReader.xml)
