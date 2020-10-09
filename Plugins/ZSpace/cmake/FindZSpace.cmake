# - Find zSpace library
# Find the zSpace includes and library
# This module defines
#  zSpace_INCLUDE_DIRS: where to find zSpace.h
#  zSpace_LIBRARIES: the zSpace library
#  zSpace_FOUND: if false, do not try to use zSpace

find_path(zSpace_INCLUDE_DIR
  NAMES
    zSpace.h
  PATHS
    C:\
  PATH_SUFFIXES
    zSpace)
mark_as_advanced(zSpace_INCLUDE_DIR)

find_library(zSpace_LIBRARY
  NAMES
    zSpace libzSpace
  PATH_SUFFIXES
    zSpace)
mark_as_advanced(zSpace_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zSpace
  REQUIRED_VARS zSpace_LIBRARY zSpace_INCLUDE_DIR)

if (zSpace_FOUND)
  set(zSpace_LIBRARIES "${zSpace_LIBRARY}")
  set(zSpace_INCLUDE_DIRS "${zSpace_INCLUDE_DIR}")
  if (NOT TARGET zSpace::zSpace)
    add_library(zSpace::zSpace UNKNOWN IMPORTED)
    set_target_properties(zSpace::zSpace
      PROPERTIES
        IMPORTED_LOCATION "${zSpace_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${zSpace_INCLUDE_DIR}")
  endif ()
endif ()
