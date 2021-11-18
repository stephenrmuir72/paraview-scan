###############################################################################
#

ExternalData_Expand_Arguments(ParaViewData _
  # data files
  "DATA{${paraview_test_data_directory_input}/Data/office.binary.vtk}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC/OfficeContour.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC/OfficeHSlice.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC/OfficeSlices.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC/OfficeVSlice.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC/ShuttleAll.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC/ShuttleZoom1.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/SurfaceLIC/ShuttleZoom2.pvsm}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf.vtm}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_0_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_1_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_2_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_3_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_4_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_5_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_6_0.vtp}"
  "DATA{${paraview_test_data_directory_input}/Data/shuttle-surf/shuttle-surf_7_0.vtp}"

  # baselines
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-OfficeContour.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-OfficeContour_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-OfficeHSlice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-OfficeVSlice.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleAll.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleAll_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleAll_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleAll_3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleAll_4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleAll_5.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom1_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom1_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom2_1.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom2_2.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom2_3.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom2_4.png}"
  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/SurfaceLIC-ShuttleZoom2_5.png}")

# Add python script names here.
set(PY_TESTS
  LockScalarRangeBackwardsCompatibility.py,NO_VALID
  SpreadSheetViewBlockNames.py,NO_VALID
  SpreadSheetViewPartialArrays.py,NO_VALID
  TransferFunctionPresets.py,NO_VALID
  TestSurfaceLIC.py,NO_VALID
  )

# Surface LIC pvbatch testing
# these tests could run safely in serial and
# in parallel.
set(PVBATCH_TESTS
  SurfaceLIC-ShuttleZoom1
  SurfaceLIC-ShuttleZoom2
  SurfaceLIC-ShuttleAll
  )

if (PARAVIEW_USE_MPI)
  # run the tests in parallel
  set(NP 3)
  set(VTK_MPIRUN_EXE mpiexec)
  # these depend on the D3 filter and
  # can't run serially
  list(APPEND PVBATCH_TESTS
    SurfaceLIC-OfficeHSlice
    SurfaceLIC-OfficeVSlice
    SurfaceLIC-OfficeSlices
    SurfaceLIC-OfficeContour
    )
  foreach (tfile ${PVBATCH_TESTS})
    ExternalData_add_test(ParaViewData
      NAME ${tfile}-Batch
      COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${NP} ${MPIEXEC_PREFLAGS}
        $<TARGET_FILE:pvbatch>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
        -D ${CMAKE_BINARY_DIR}/ExternalData/Testing/Data
        -T ${CMAKE_BINARY_DIR}/Testing/Temporary
        -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${tfile}.png}
        --threshold 30
        --state DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/${tfile}.pvsm})
  endforeach()
  set(SurfaceLIC-OfficeSlices_THRESHOLD 30)
  set(SurfaceLIC-OfficeVSlice_THRESHOLD 30)
else()
  # run the test serially
  foreach (tfile ${PVBATCH_TESTS})
    ExternalData_add_test(ParaViewData
      NAME ${tfile}-Batch
      COMMAND $<TARGET_FILE:pvbatch>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceLICTestDriver.py
        -D ${CMAKE_BINARY_DIR}/ExternalData/Testing/Data
        -T ${CMAKE_BINARY_DIR}/Testing/Temporary
        -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/${tfile}.png}
        --threshold 30
        --state DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/${tfile}.pvsm})
  endforeach()
endif()

paraview_add_test_python(
  JUST_VALID
  ${PY_TESTS}
  )
