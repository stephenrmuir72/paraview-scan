set(classes
  vtk3MFReader)

set(private_classes
  vtk3MFZipArchive)

vtk_module_add_module(ThreeMFReader::vtk3MFReader
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})

paraview_add_server_manager_xmls(
  XMLS  ThreeMFReader.xml)
