set(paraview_contract_file_urls
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CFullExample.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxFullExample.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxImageDataExample.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxMultiChannelInputExample.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxMultimesh.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxOverlappingAMRExample.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxPolygonalWithAttributes.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxPolyhedraExample.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-CxxSteeringExample.cmake"
  "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-Fortran90FullExample.cmake"
)

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "python")
  list(APPEND paraview_contract_file_urls
    "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-PythonFullExample.cmake"
    "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/master/CMake/contract-tests/paraview-PythonSteeringExample.cmake")
endif ()



set(PARAVIEW_CONTRACT_FILE_URLS ${paraview_contract_file_urls} CACHE STRING "")
