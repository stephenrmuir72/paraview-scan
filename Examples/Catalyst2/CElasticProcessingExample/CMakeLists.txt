cmake_minimum_required(VERSION 3.13)
project(CElasticProcessingExample LANGUAGES C)

#------------------------------------------------------------------------------
add_executable(CElasticProcessingExample
  FEDataStructures.c
  FEDataStructures.h
  FEDriver.c)

# Since this example uses MPI, find and link against it.
find_package(MPI COMPONENTS C REQUIRED)
target_link_libraries(CElasticProcessingExample
  PRIVATE
    MPI::MPI_C)

#------------------------------------------------------------------------------
option(USE_CATALYST "Build example with Catalyst enabled" ON)
if (USE_CATALYST)
  find_package(catalyst REQUIRED
    PATHS "${ParaView_DIR}/catalyst")
  target_compile_definitions(CElasticProcessingExample
    PRIVATE
      USE_CATALYST=1)
  target_link_libraries(CElasticProcessingExample
    PRIVATE
      catalyst::catalyst)

  include(CTest)
  if (BUILD_TESTING)
    add_test(
      NAME CElasticProcessingExample::SimplePipeline
      COMMAND CElasticProcessingExample
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_pipeline.py)
    set_tests_properties("CElasticProcessingExample::SimplePipeline"
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "ERROR:;instances(s)? still around;ERR\\\|"
        SKIP_RETURN_CODE 125)
  endif()
endif()
