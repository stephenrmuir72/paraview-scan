cmake_minimum_required (VERSION 3.3)
project (CatalystFortran03FullExample Fortran CXX)

find_package (ParaView 4.1 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include (${PARAVIEW_USE_FILE})

if (NOT PARAVIEW_USE_MPI)
  message (SEND_ERROR "ParaView must be built with MPI enabled")
endif ()
if(NOT MPI_Fortran_LIBRARIES)
	find_package (MPI)
endif ()

add_library (CxxAdaptor STATIC FECxxAdaptor.cxx)

include_directories (${MPI_Fortran_INCLUDE_PATH})
link_directories (${MPI_Fortran_LIBRARIES})

add_executable (Fortran03FullExample FEDriver.f90 FEFortranAdaptor.f90 Interfaces.f90)
target_link_libraries (Fortran03FullExample vtkPVPythonCatalyst CxxAdaptor ${MPI_Fortran_LIBRARIES})

set_target_properties (Fortran03FullExample PROPERTIES LINKER_LANGUAGE Fortran)

option (BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/coproc.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  include( CTest)
  add_test (NAME Fortran03FullExampleTest COMMAND Fortran03FullExample)
  set_tests_properties (Fortran03FullExampleTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif ()
