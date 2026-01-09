set(classes
  vtkAbstractAccumulator
  vtkAbstractArrayMeasurement
  vtkArithmeticMeanArrayMeasurement
  vtkEntropyArrayMeasurement
  vtkGeometricMeanArrayMeasurement
  vtkHarmonicMeanArrayMeasurement
  vtkMaxAccumulator
  vtkMaxArrayMeasurement
  vtkQuantileAccumulator
  vtkQuantileArrayMeasurement
  vtkResampleToHyperTreeGrid
  vtkStandardDeviationArrayMeasurement)

set(headers
  vtkFunctor.h)

set(template_classes
  vtkArithmeticAccumulator
  vtkBinsAccumulator)

vtk_module_add_module(HyperTreeGridFilters
  FORCE_STATIC  # build static library, to avoid confusion when loading
  CLASSES ${classes}
  HEADERS ${headers}
  TEMPLATE_CLASSES ${template_classes})

paraview_add_server_manager_xmls(
  XMLS  HyperTreeGridFilters.xml)
