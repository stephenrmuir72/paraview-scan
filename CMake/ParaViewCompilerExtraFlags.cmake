if (CMAKE_COMPILER_IS_GNUCXX)

  include(CheckCXXCompilerFlag)

  # Additional warnings for GCC
  set(paraview_extra_warning_flags
    -Wnon-virtual-dtor
    -Wno-long-long
    -Wcast-align
    -Wchar-subscripts
    -Wall
    -Wextra
    -Wpointer-arith
    -Wformat-security
    # GCC 16's -Wfree-nonheap-object false-positives on inlined std::vector
    # deallocation paths in heavily-templated code (e.g. Viskores' Buffer
    # storage). Known upstream false-positive category; a libstdc++ RAII
    # fix has been proposed but is not yet universally landed:
    # https://gcc.gnu.org/pipermail/libstdc++/2024-April/058552.html
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115016
    -Wno-free-nonheap-object
    -Woverloaded-virtual
    -Wshadow
    # GCC's -Wsfinae-incomplete (new in GCC 16, also seen on GCC 15 trunk builds)
    # fires on Qt's own MOC-generated code and forward-declared Qt/VTK types used
    # in Qt property/signal contexts -- a pattern Qt itself recommends. Reported
    # upstream as likely a false positive:
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121244
    # See also: https://forum.qt.io/topic/164690/gcc-16-warnings-about-incomplete-types-in-an-sfinae-context
    -Wno-sfinae-incomplete
    -Wunused-parameter
    -fno-check-new)

  # This flag is useful as not returning from a non-void function is an error
  # with MSVC, but it is not supported on all GCC compiler versions
  check_cxx_compiler_flag(-Werror=return-type HAVE_GCC_ERROR_RETURN_TYPE)
  if (HAVE_GCC_ERROR_RETURN_TYPE)
    list(APPEND paraview_extra_warning_flags
      -Werror=return-type)
  endif ()

  # If we are compiling on Linux then set some extra linker flags too
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(PARAVIEW_LINKER_FATAL_WARNINGS "Specify if linker warnings must be considered as errors." OFF)
    mark_as_advanced(PARAVIEW_LINKER_FATAL_WARNINGS)
    if (TARGET paraviewbuild)
      # XXX(cmake-3.13): use `target_link_options`
      set_property(TARGET paraviewbuild APPEND
        PROPERTY
          INTERFACE_LINK_OPTIONS
            "$<$<BOOL:${PARAVIEW_LINKER_FATAL_WARNINGS}>:LINKER:--fatal-warnings>")
    endif ()
  endif ()

  # Set up the CXX_FLAGS for extra warnings
  option(PARAVIEW_EXTRA_COMPILER_WARNINGS
    "Add compiler flags to do stricter checking" OFF)
  if (PARAVIEW_EXTRA_COMPILER_WARNINGS)
    if (TARGET paraviewbuild)
      target_compile_options(paraviewbuild
        INTERFACE
          "$<$<COMPILE_LANGUAGE:CXX>:${paraview_extra_warning_flags}>")
    endif ()
  endif ()
endif ()

# Intel OneAPI compilers >= 2021.2.0 turn on "fast math" at any non-zero
# optimization level. Suppress this non-standard behavior using the
# `-fp-model=precise` flag.
set(intel_oneapi_compiler_detections)
set(intel_oneapi_compiler_version_min "2021.2.0")
foreach (lang IN ITEMS C CXX Fortran)
  if (CMAKE_VERSION VERSION_LESS "3.14" AND lang STREQUAL "Fortran") # XXX(cmake-3.14): `Fortran_COMPILER_ID` genex
    continue ()
  endif ()
  # Detect the IntelLLVM compiler for the given language.
  set(is_lang "$<COMPILE_LANGUAGE:${lang}>")
  set(is_intelllvm "$<${lang}_COMPILER_ID:IntelLLVM>")
  set(is_intelllvm_fastmath_assuming_version "$<VERSION_GREATER_EQUAL:$<${lang}_COMPILER_VERSION>,${intel_oneapi_compiler_version_min}>")
  list(APPEND intel_oneapi_compiler_detections
    "$<AND:${is_lang},${is_intelllvm},${is_intelllvm_fastmath_assuming_version}>")
endforeach ()
string(REPLACE ";" "," intel_oneapi_compiler_detections "${intel_oneapi_compiler_detections}")
if (TARGET paraviewbuild)
  target_compile_options(paraviewbuild
    INTERFACE
      "$<BUILD_INTERFACE:$<$<OR:${intel_oneapi_compiler_detections}>:-fp-model=precise>>")
endif ()
