set(paraview_contract_file_urls
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CUnstructuredGrid.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxImageData.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxMultiChannelInput.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxMultimesh.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxOverlappingAMR.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxPolygonalWithAttributes.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxPolyhedra.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxSteering.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxUnstructuredGrid.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-Fortran90ImageData.cmake"
)

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "python")
  list(APPEND paraview_contract_file_urls
    "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-PythonImageData.cmake"
    "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-PythonSteering.cmake")
endif ()



set(PARAVIEW_CONTRACT_FILE_URLS ${paraview_contract_file_urls} CACHE STRING "")
