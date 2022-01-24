set(classes
  vtkCDIReader)

set (sources
    cdi_tools.cxx
    cdi_tools.h
    helpers.cxx
    helpers.h
    projections.cxx
    projections.h
    )

vtk_module_add_module(CDIReader::vtkCDIReader
  CLASSES ${classes}
  SOURCES ${sources}
)

vtk_module_find_package(
  PACKAGE CDI
  VERSION 2)

vtk_module_link(CDIReader::vtkCDIReader
  PRIVATE
  CDI::CDI)

paraview_add_server_manager_xmls(
  XMLS  CDIReader.xml)
