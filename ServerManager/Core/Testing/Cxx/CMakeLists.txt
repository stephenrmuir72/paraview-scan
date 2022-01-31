 vtk_add_test_cxx(vtkServerManagerCoreCxxTests tests
   NO_DATA NO_VALID NO_OUTPUT
   #  TestPartialArraysInformation.cxx
   # TestPVArrayInformation.cxx
   # TestSpecialDirectories.cxx
   # TestProxy.cxx
   TestDomainIterator.cxx
   TestPropertyIterator.cxx
   TestProxyDefinitionsIterator.cxx
   TestProxyManagerIterator.cxx
   )

vtk_add_test_cxx(vtkServerManagerCoreCxxTests tests
  --client
  TestClientApplication,TestPVApplication.cxx,NO_VALID)

vtk_add_test_cxx(vtkServerManagerCoreCxxTests tests
  --server
  TestServerApplication,TestPVApplication.cxx,NO_VALID)

# TODO
#vtk_add_test_cxx(vtkServerManagerCoreCxxTests tests
#  --batch
#  TestBatchApplication,TestPVApplication.cxx,NO_VALID)

vtk_test_cxx_executable(vtkServerManagerCoreCxxTests tests)
