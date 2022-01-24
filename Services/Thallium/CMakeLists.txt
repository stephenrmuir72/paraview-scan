set(classes
  vtkThalliumService
  vtkThalliumServiceEndpoint
  vtkThalliumServicesEngine)

set(private_classes
  vtkThalliumServicesEngineInternals
  vtkThalliumRunLoop
  vtkThalliumEventualImplementation)

set(private_headers)

set(remoting_overrides
  Service
  ServiceEndpoint
  ServicesEngine)

foreach (override IN LISTS remoting_overrides)
  vtk_object_factory_declare(
    BASE "vtk${override}"
    OVERRIDE "vtkThallium${override}")
endforeach()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKSERVICESTHALLIUM_EXPORT")

vtk_module_add_module(ParaView::ServicesThallium
  CLASSES         ${classes}
  PRIVATE_CLASSES ${private_classes}
  SOURCES         ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header}
                  ${private_headers})

vtk_module_remoting_exclude()
vtk_module_find_package(PACKAGE thallium)
vtk_module_link(ParaView::ServicesThallium
  PRIVATE
    thallium)
