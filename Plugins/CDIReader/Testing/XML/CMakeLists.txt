# CDIReader Plugin testing
# these tests could run safely in serial and
# in parallel.

# ExternalData_Expand_Arguments(ParaViewData _
#  "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/NetCDF/ts.nc}"
# "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/NetCDF/fesom.nc}"
# "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/GRIB/ts.grib}"
#   "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/GRIB/grid.nc}"
# )

if (NOT PARAVIEW_USE_QT)
  message(STATUS "CDI tests require Qt. Skipping.")
endif()

set (xml_tests_no_final_verif
  # edges.xml
  fesom.xml
  grib.xml
  ts.xml
  )


paraview_add_client_tests(
  LOAD_PLUGIN CDIReader
  #    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline
  TEST_DATA_TARGET ParaViewData
  PREFIX CDIReaderPlugin::pv
  TEST_SCRIPTS ${xml_tests_no_final_verif}
  )

paraview_add_client_server_tests(
  LOAD_PLUGIN CDIReader
  #    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline
  TEST_DATA_TARGET ParaViewData
  PREFIX CDIReaderPlugin::pvcs
  TEST_SCRIPTS ${xml_tests_no_final_verif}
  )
