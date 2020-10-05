# Set some flags to avoid copious warnings.
#   - C4251: warnings about dll-interface of inherited classes
set(CMAKE_CXX_FLAGS "/wd4251" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_windows.cmake")
