cmake_minimum_required(VERSION 3.13)
project(CxxDirectConduitExampleV2 LANGUAGES C CXX)

include (GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

#------------------------------------------------------------------------------
# since we use C++11 in this example.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(-Dconduit_c_EXPORTS) # probably not needed but this is how conduit_node.cpp is compiled

# Since this example uses MPI, find and link against it.
find_package(MPI COMPONENTS C CXX)
if (NOT MPI_FOUND)
  message(STATUS
    "Skipping example: ${PROJECT_NAME} requires MPI.")
  return ()
endif ()

option(USE_CATALYST "Build example with Catalyst enabled" ON)
set(SOURCE_CODE
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (USE_CATALYST)
   list (APPEND SOURCE_CODE CatalystAdaptor.h CatalystAdaptor.cxx ConduitAdaptor.h)
endif()

#------------------------------------------------------------------------------
add_executable(CxxDirectConduitExampleV2
        ${SOURCE_CODE} )
target_link_libraries(CxxDirectConduitExampleV2
  PRIVATE
    MPI::MPI_C
    MPI::MPI_CXX)

find_library(catalystdouble libcatalyst-double.so /home/acbauer/Code/ParaView/Catalyst/debug/lib64/catalyst)
find_library(catalyststub libcatalyst-stub.so /home/acbauer/Code/ParaView/Catalyst/debug/lib64/catalyst)


#------------------------------------------------------------------------------
if (USE_CATALYST)
  find_package(catalyst REQUIRED
    PATHS "${ParaView_DIR}/catalyst")
  target_compile_definitions(CxxDirectConduitExampleV2
    PRIVATE
      "PARAVIEW_IMPL_DIR=\"${ParaView_CATALYST_DIR}\""
      USE_CATALYST=1)
  target_link_libraries(CxxDirectConduitExampleV2
    PRIVATE
    catalyst::blueprint_headers
    catalyst::conduit_headers
    catalyst::catalyst_headers
      catalyst::catalyst
      ${catalystdouble}
      ${catalyststub}
)

  include(CTest)
  if (BUILD_TESTING)
    add_test(
      NAME CxxDirectConduitExampleV2::SimplePipeline
      COMMAND CxxDirectConduitExampleV2
              ${CMAKE_CURRENT_SOURCE_DIR}/catalyst_pipeline.py)

    set(_vtk_fail_regex
      # CatalystAdaptor
      "Failed"
      # vtkLogger
      "(\n|^)ERROR: "
      "ERR\\|"
      # vtkDebugLeaks
      "instance(s)? still around")

    set_tests_properties("CxxDirectConduitExampleV2::SimplePipeline"
      PROPERTIES
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        PASS_REGULAR_EXPRESSION "executing catalyst_pipeline"
        SKIP_REGULAR_EXPRESSION "Python support not enabled"
        SKIP_RETURN_CODE 125)
  endif()
endif()
