# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindzSpace
--------

Find zSpace.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``zSpace::zSpace``
if zSpace has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``zSpace_FOUND``
  True if zSpace is found.
``zSpace_INCLUDE_DIRS``
  Include directories for zSpace headers.
``zSpace_LIBRARIES``
  Libraries to link to zSpace.
``zSpace_VERSION``
  The version of zSpace found.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``zSpace_LIBRARY``
  The libzSpace library file.
``zSpace_INCLUDE_DIR``
  The directory containing ``zSpace.h``.

Hints
^^^^^

Set ``zSpace_DIR`` or ``zSpace_ROOT`` in the environment to specify the
zSpace installation prefix.
#]=======================================================================]

# $zSpaceDIR is an environment variable that would
# correspond to the ./configure --prefix=$zSpace_DIR
# used in building zSpace.
#
# This makes the presumption that you are include zSpace.h like
#
#include "zSpace.h"

find_path(zSpace_INCLUDE_DIR zSpace.h
  HINTS
    ${zSpace_ROOT}
    ENV zSpace_DIR
    ENV zSpace_ROOT
  PATH_SUFFIXES
    include/zSpace
    include/zSpace
    include
  DOC "Path to the zSpace include directory"
)
mark_as_advanced(zSpace_INCLUDE_DIR)

# zSpace name its library when built with CMake as `zSpace${major}${minor}`.
set(_zSpace_versions
    3.0 2.4 2.3 2.2 2.1 2.0 1.11 1.10 1.9 1.8 1.7 1.6 1.5 1.4 1.3 1.2)

set(_zSpace_libnames)
foreach (_zSpace_version IN LISTS _zSpace_versions)
    string(REPLACE "." "" _zSpace_version "${_zSpace_version}")
    list(APPEND _zSpace_libnames "zSpace${_zSpace_version}" "zSpace${_zSpace_version}")
endforeach ()

find_library(zSpace_LIBRARY
  NAMES ${_zSpace_libnames} zSpace zSpace_i zSpace1.5.0 zSpace1.4.0 zSpace1.3.2 zSpace
  HINTS
    ${zSpace_ROOT}
    ENV zSpace_DIR
    ENV zSpace_ROOT
  DOC "Path to the zSpace library"
)
mark_as_advanced(zSpace_LIBRARY)

if (EXISTS "${zSpace_INCLUDE_DIR}/zSpace_version.h")
    file(STRINGS "${zSpace_INCLUDE_DIR}/zSpace_version.h" _zSpace_version
        REGEX "zSpace_RELEASE_NAME")
    string(REGEX REPLACE ".*\"\(.*\)\"" "\\1" zSpace_VERSION "${_zSpace_version}")
    unset(_zSpace_version)
else ()
    set(zSpace_VERSION zSpace_VERSION-NOTFOUND)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zSpace
    VERSION_VAR zSpace_VERSION
    REQUIRED_VARS zSpace_LIBRARY zSpace_INCLUDE_DIR)

if (zSpace_FOUND)
    set(zSpace_LIBRARIES ${zSpace_LIBRARY})
    set(zSpace_INCLUDE_DIRS ${zSpace_INCLUDE_DIR})

    if (NOT TARGET zSpace::zSpace)
        add_library(zSpace::zSpace UNKNOWN IMPORTED)
        set_target_properties(zSpace::zSpace PROPERTIES
            IMPORTED_LOCATION "${zSpace_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${zSpace_INCLUDE_DIR}")
    endif ()
endif ()
