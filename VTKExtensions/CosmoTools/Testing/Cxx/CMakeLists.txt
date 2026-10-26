# FIXME: these tests are not designed for parallel MPI runs!
# we need to fix them before we can run them on more than 1 rank.
# see paraview/paraview#18813
set (vtkPVVTKExtensionsCosmoToolsCxxTests_NUMPROCS 1)

vtk_add_test_mpi(vtkPVVTKExtensionsCosmoToolsCxxTests tests
  TESTING_DATA
  TestHaloFinder.cxx # test of particles output
  TestHaloFinderSummaryInfo.cxx # test of summary information output
  TestHaloFinderSubhaloFinding.cxx # test of subhalo finding option
  TestSubhaloFinder.cxx # test of subhalo finding filter
)

# Uses real particle data but compares the results of the filters for several
# input types, so there is no baseline image.
vtk_add_test_mpi(vtkPVVTKExtensionsCosmoToolsCxxTests tests
  TESTING_DATA
  NO_VALID
  TestHaloFinderCompositeTypes.cxx # test of vtkUnstructuredGrid/vtkMultiBlockDataSet/vtkPartitionedDataSetCollection input
)

# Uses synthetic in-memory data, so it does not need TESTING_DATA.
vtk_add_test_mpi(vtkPVVTKExtensionsCosmoToolsCxxTests tests
  NO_VALID
  TestPMergeConnectedCompositeTypes.cxx # test of vtkMultiBlockDataSet/vtkPartitionedDataSetCollection input/output
)

# Uses synthetic in-memory data, but needs TESTING_DATA for the -T temporary
# output directory argument (see vtkTestUtilities::GetTemporaryDir).
vtk_add_test_mpi(vtkPVVTKExtensionsCosmoToolsCxxTests tests
  TESTING_DATA
  NO_VALID
  TestGenericIOMultiBlockWriterCompositeTypes.cxx # test of vtkMultiBlockDataSet/vtkPartitionedDataSetCollection input
)

vtk_test_cxx_executable(vtkPVVTKExtensionsCosmoToolsCxxTests tests
HaloFinderTestHelpers.h
)
