# Set miminum required version of cmake for this project.
cmake_minimum_required(VERSION 3.3)

# Specify workspace name.
project(CatalystFortran03FullExample Fortran CXX)

# Find and load settings from external ParaView project.
find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVPythonCatalyst)

# Set include directories of required Paraview header files.
include(${PARAVIEW_USE_FILE})

if(NOT PARAVIEW_USE_MPI)
  message(SEND_ERROR "ParaView must be built with MPI enabled")
endif()
if(NOT MPI_Fortran_LIBRARIES)
  find_package(MPI)
endif()

# Adds include directory to the build (mpifptr.h).
include_directories ("C:/Program Files (x86)/Microsoft SDKs/MPI/Include/x64")
# include_directories(${MPI_Fortran_INCLUDE_PATH}/x64) # mpifptr.h, doesn't work

# Get directory name of MPI library (to find location of msmpifec.lib).
get_filename_component (mpi_lib_dir ${MPI_Fortran_LIBRARIES} DIRECTORY)

# Adds CxxAdaptor.lib to project using source file FECxxAdaptor.cxx.
add_library(CxxAdaptor STATIC FECxxAdaptor.cxx)

# Add following executable target to this build process.
add_executable(Fortran03FullExample FEDriver.f90 FEFortranAdaptor.f90 Interfaces.f90)

# Link target Fortran90FullExample to given libraries.
target_link_libraries(Fortran03FullExample vtkPVPythonCatalyst ${MPI_Fortran_LIBRARIES} ${mpi_lib_dir}/msmpifec.lib CxxAdaptor)

# Set linker language as Fortran.
set_target_properties(Fortran03FullExample PROPERTIES LINKER_LANGUAGE Fortran)

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/coproc.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  include(CTest)
  add_test(NAME Fortran03FullExampleTest COMMAND Fortran03FullExample)
  set_tests_properties(Fortran03FullExampleTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
