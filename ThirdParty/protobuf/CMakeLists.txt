# FIXME: export symbols on non-Windows as well.
set(CMAKE_CXX_VISIBILITY_PRESET "default")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 0)

set(protobuf_config_arg)
if (NOT BUILD_SHARED_LIBS)
  # Only force config mode for static builds, so that transitive dependencies
  # are properly resolved. Shared builds can fall back to module mode.
  set(protobuf_config_arg "CONFIG_MODE")
endif ()

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkprotobuf/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2008 Google Inc."
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/protobuf.git@for/paraview-20260206-3.13.0"
    VERSION
      "3.13.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Protobuf
    ${protobuf_config_arg}
    TARGETS protobuf::libprotobuf
    STANDARD_INCLUDE_DIRS)

if (VTK_MODULE_USE_EXTERNAL_ParaView_protobuf)
  # Omitted the VERSION specifier in the EXTERNAL block above because
  # protobuf package checks reject v24.x (the version available in the
  # superbuild) when v3.4 is requested. Also, the Protobuf_VERSION variable
  # doesn't survive the vtk_module_third_party() function, so we can't use
  # that. It seems the options are either regex parse an internal cache
  # string variable, or explicitly query the protoc executable.

  if (Protobuf_PROTOC_EXECUTABLE)
    execute_process(
      COMMAND "${Protobuf_PROTOC_EXECUTABLE}" --version
      ERROR_QUIET
      OUTPUT_VARIABLE _protoc_version_output
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _protoc_result
    )

    # Output typically looks like "libprotoc 3.12.4" or "libprotoc 24.1.0"
    if (_protoc_result EQUAL 0 AND _protoc_version_output MATCHES "([0-9]+\\.[0-9]+\\.[0-9]+)")
      set(extracted_pb_version "${CMAKE_MATCH_1}")
      if (extracted_pb_version VERSION_LESS "3.4")
        message(FATAL_ERROR "ParaView requires Protobuf version 3.4 or newer. Found version: ${extracted_pb_version}")
      else()
        message(STATUS "Version ${extracted_pb_version} of protobuf is suitable!")
      endif ()
    else ()
      message(WARNING "Could not parse Protobuf version from protoc output: ${_protoc_version_output}")
    endif ()
  else ()
    message(WARNING "Protobuf_PROTOC_EXECUTABLE not found. Cannot verify Protobuf >= 3.4.")
  endif ()
endif ()

if (VTK_MODULE_USE_EXTERNAL_ParaView_protobuf)
  # promote protobuf::protoc to GLOBAL to allow to call protobuf_generate from other directories
  set_target_properties (protobuf::protoc PROPERTIES IMPORTED_GLOBAL TRUE)
  set(protobuf_function_file "${CMAKE_CURRENT_SOURCE_DIR}/protobuf-function.cmake")
else ()
  set(protobuf_function_file "${CMAKE_CURRENT_SOURCE_DIR}/vtkprotobuf/cmake/protobuf-function.cmake")
endif ()
set(protobuf_function_file_output
  "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_build_CMAKE_DESTINATION}/protobuf-function.cmake")
configure_file(
  "${protobuf_function_file}"
  "${protobuf_function_file_output}"
  COPYONLY)
include("${protobuf_function_file_output}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_protobuf.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_protobuf.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_protobuf.h")
