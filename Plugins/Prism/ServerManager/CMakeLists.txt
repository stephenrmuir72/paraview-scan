set(classes
  vtkSMPrismIsSimulationDataDomain
  vtkSMPrismTableArraysDomain
  vtkSMPrismThresholdRangeDomain
  )

vtk_module_add_module(Prism::vtkPrismServerManager
  CLASSES ${classes}
  )

# FIXME vtkSMPrismThresholdRangeDomain.cxx:34:3: error: redefinition of ‘template<class T> T {anonymous}::lexical_cast(const std::string&)’
set_property(SOURCE vtkSMPrismThresholdRangeDomain.cxx
  PROPERTY
    SKIP_UNITY_BUILD_INCLUSION ON)
