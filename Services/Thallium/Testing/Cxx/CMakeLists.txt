vtk_add_test_cxx(vtkServicesThalliumCxxTests tests
  NO_VALID
  #  PublishSubscribeAlgorithms.cxx
  TestBasicRemoting.cxx
  TestChannels.cxx
  TestEndpointUnsubscribe.cxx
  #TestSerialization.cxx
  #TestService2ServiceDataStream.cxx
  TestInterruptPipeline.cxx
  )
vtk_test_cxx_executable(vtkServicesThalliumCxxTests tests)
