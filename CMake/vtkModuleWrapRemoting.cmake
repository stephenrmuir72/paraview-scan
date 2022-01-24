#[==[.md
# `vtkModuleWrapRemoting`

TODO
#]==]

#[==[.md INTERNAL
## Wrapping a single module

This function generates the wrapped sources for a module. It places the list of
generated source files and classes in variables named in the second and third
arguments, respectively.

```
_vtk_module_wrap_remoting_sources(<module> <sources> <classes>)
```
#]==]

cmake_policy(PUSH)
cmake_policy(SET CMP0053 NEW)

function (_vtk_module_wrap_remoting_sources module sources classes)
  _vtk_module_get_module_property("${module}"
    PROPERTY  "exclude_wrap"
    VARIABLE  _vtk_remoting_exclude_wrap)
  if (_vtk_remoting_exclude_wrap)
    return ()
  endif ()
  _vtk_module_get_module_property("${module}"
    PROPERTY  "remoting_exclude"
    VARIABLE  _vtk_remoting_exclude)
  if (_vtk_remoting_exclude)
    return ()
  endif ()

  set(_vtk_remoting_args_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_remoting_library_name}-RXX.$<CONFIGURATION>.args")

  set(_vtk_remoting_genex_compile_definitions
    "$<TARGET_PROPERTY:${_vtk_remoting_target_name},COMPILE_DEFINITIONS>")
  set(_vtk_remoting_genex_include_directories
    "$<TARGET_PROPERTY:${_vtk_remoting_target_name},INCLUDE_DIRECTORIES>")
  file(GENERATE
    OUTPUT  "${_vtk_remoting_args_file}"
    CONTENT "$<$<BOOL:${_vtk_remoting_genex_compile_definitions}>:\n-D\'$<JOIN:${_vtk_remoting_genex_compile_definitions},\'\n-D\'>\'>\n
$<$<BOOL:${_vtk_remoting_genex_include_directories}>:\n-I\'$<JOIN:${_vtk_remoting_genex_include_directories},\'\n-I\'>\'>\n")

  _vtk_module_get_module_property("${module}"
    PROPERTY  "hierarchy"
    VARIABLE  _vtk_remoting_hierarchy_file)

  get_property(_vtk_remoting_is_imported
    TARGET    "${_vtk_remoting_target_name}"
    PROPERTY  "IMPORTED")
  if (_vtk_remoting_is_imported OR CMAKE_GENERATOR MATCHES "Ninja")
    set(_vtk_remoting_command_depend "${_vtk_remoting_hierarchy_file}")
  else ()
    if (TARGET "${_vtk_remoting_library_name}-hierarchy")
      set(_vtk_remoting_command_depend "${_vtk_remoting_library_name}-hierarchy")
    else ()
      message(FATAL_ERROR
        "The ${module} hierarchy file is attached to a non-imported target "
        "and a hierarchy target "
        "(${_vtk_remoting_library_name}-hierarchy) is missing.")
    endif ()
  endif ()

  # create directory for wrapped source files
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_remoting_library_name}RXX")

  set(_vtk_remoting_sources)

  _vtk_module_get_module_property("${module}"
    PROPERTY  "headers"
    VARIABLE  _vtk_remoting_headers)
  set(_vtk_remoting_classes)
  foreach (_vtk_remoting_header IN LISTS _vtk_remoting_headers)
    get_filename_component(_vtk_remoting_basename "${_vtk_remoting_header}" NAME_WE)
    list(APPEND _vtk_remoting_classes
      "${_vtk_remoting_basename}")

    set(_vtk_remoting_source_output
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_remoting_library_name}RXX/${_vtk_remoting_basename}RXX.cxx")
    list(APPEND _vtk_remoting_sources
      "${_vtk_remoting_source_output}")

    add_custom_command(
      OUTPUT  "${_vtk_remoting_source_output}"
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
              "$<TARGET_FILE:ParaView::WrapRemoting>"
              "@${_vtk_remoting_args_file}"
              -o "${_vtk_remoting_source_output}"
              "${_vtk_remoting_header}"
              --types "${_vtk_remoting_hierarchy_file}"
      IMPLICIT_DEPENDS
              CXX "${_vtk_remoting_header}"
      COMMENT "Generating remoting wrapper sources for ${_vtk_remoting_basename}"
      DEPENDS
        "$<TARGET_FILE:ParaView::WrapRemoting>"
        "${_vtk_remoting_header}"
        "${_vtk_remoting_args_file}"
        "${_vtk_remoting_command_depend}")
  endforeach ()

  set("${sources}"
    "${_vtk_remoting_sources}"
    PARENT_SCOPE)
  set("${classes}"
    "${_vtk_remoting_classes}"
    PARENT_SCOPE)
endfunction ()

#[==[.md INTERNAL
## Generating a client server library

A client server library may consist of the wrappings of multiple VTK modules.
This is useful for kit-based builds where the modules part of the same kit
belong to the same client server library as well.

```
_vtk_module_wrap_remoting_library(<name> <module>...)
```

The first argument is the name of the client server library. The remaining
arguments are VTK modules to include in the library.

The remaining information it uses is assumed to be provided by the
`vtk_module_wrap_remoting` function.
#]==]
function (_vtk_module_wrap_remoting_library name)
  set(_vtk_remoting_library_sources)
  set(_vtk_remoting_library_classes)
  foreach (_vtk_remoting_module IN LISTS ARGN)
    _vtk_module_get_module_property("${_vtk_remoting_module}"
      PROPERTY  "exclude_wrap"
      VARIABLE  _vtk_remoting_exclude_wrap)
    if (_vtk_remoting_exclude_wrap)
      continue ()
    endif ()
    _vtk_module_get_module_property("${_vtk_remoting_module}"
      PROPERTY  "remoting_exclude"
      VARIABLE  _vtk_remoting_exclude)
    if (_vtk_remoting_exclude)
      return ()
    endif ()

    _vtk_module_wrap_remoting_sources("${_vtk_remoting_module}" _vtk_remoting_sources _vtk_remoting_classes)
    list(APPEND _vtk_remoting_library_sources
      ${_vtk_remoting_sources})
    list(APPEND _vtk_remoting_library_classes
      ${_vtk_remoting_classes})
  endforeach ()

  if (NOT _vtk_remoting_library_sources)
    return ()
  endif ()

  set(_vtk_remoting_declarations)
  set(_vtk_remoting_calls)
  foreach (_vtk_remoting_class IN LISTS _vtk_remoting_library_classes)
    string(APPEND _vtk_remoting_declarations
      " extern void ${_vtk_remoting_class}RXX();\n")
    string(APPEND _vtk_remoting_calls
      "  ${_vtk_remoting_class}RXX();\n")
  endforeach ()
  set(_vtk_remoting_init_content
  "#include \"vtkABI.h\"
  ${_vtk_remoting_declarations}

  extern \"C\" void VTK_ABI_EXPORT ${name}_Initialize()
  {
  ${_vtk_remoting_calls}}\n")


  set(_vtk_remoting_init_file
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${name}Init.cxx")
  file(GENERATE
    OUTPUT  "${_vtk_remoting_init_file}"
    CONTENT "${_vtk_remoting_init_content}")
  # XXX(cmake): Why is this necessary? One would expect that `file(GENERATE)`
  # would do this automatically.
  set_property(SOURCE "${_vtk_remoting_init_file}"
    PROPERTY
      GENERATED 1)

  add_library("${name}" STATIC
    ${_vtk_remoting_library_sources}
    "${_vtk_remoting_init_file}")
  if (BUILD_SHARED_LIBS)
    set_property(TARGET "${name}"
      PROPERTY
        POSITION_INDEPENDENT_CODE 1)
  endif ()
  set(_vtk_build_LIBRARY_NAME_SUFFIX "${_vtk_remoting_LIBRARY_NAME_SUFFIX}")
  set(_vtk_build_ARCHIVE_DESTINATION "${_vtk_remoting_DESTINATION}")
  _vtk_module_apply_properties("${name}")

  vtk_module_autoinit(
    MODULES ${ARGN}
    TARGETS "${name}")

  target_link_libraries("${name}"
    PRIVATE
      ${ARGN}
      ParaView::ServerManagerReflection)

  set(_vtk_remoting_export)
  if (_vtk_remoting_INSTALL_EXPORT)
    list(APPEND _vtk_remoting_export
      EXPORT "${_vtk_remoting_INSTALL_EXPORT}")
  endif ()

  install(
    TARGETS             "${name}"
    ${_vtk_remoting_export}
    COMPONENT           "${_vtk_remoting_COMPONENT}"
    ARCHIVE DESTINATION "${_vtk_remoting_DESTINATION}")
endfunction ()

#[==[.md
## Wrapping a set of VTK modules for Remoting

```
vtk_module_wrap_remoting(
  MODULES <module>...
  TARGET <target>
  [WRAPPED_MODULES <varname>]

  [FUNCTION_NAME <function>]
  [DESTINATION <destination>]

  [INSTALL_EXPORT <export>]
  [COMPONENT <component>])
```

  * `MODULES`: (Required) The list of modules to wrap.
  * `TARGET`: (Required) The target to create which represents all wrapped
    Remoting modules. This is used to provide the function used to
    initialize the bindings.
  * `WRAPPED_MODULES`: (Recommended) Not all modules are wrappable. This
    variable will be set to contain the list of modules which were wrapped.
  * `FUNCTION_NAME`: (Recommended) (Defaults to `<TARGET>_initialize`) The
    function name to generate in order to initialize the client server
    bindings.A header with the name `<TARGET>.h` should be included in order to
    access the initialization function.
  * `DESTINATION`: (Defaults to `${CMAKE_INSTALL_LIBDIR}`) Where to install the
    generated libraries.
  * `INSTALL_EXPORT`: If provided, installs will add the installed
    libraries and generated interface target to the provided export set.
  * `COMPONENT`: (Defaults to `development`) All install rules created by this
    function will use this installation component.
#]==]
function (vtk_module_wrap_remoting)
  cmake_parse_arguments(_vtk_remoting
    ""
    "DESTINATION;INSTALL_EXPORT;TARGET;COMPONENT;FUNCTION_NAME;WRAPPED_MODULES"
    "MODULES"
    ${ARGN})

  if (_vtk_remoting_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_wrap_remoting: "
      "${_vtk_remoting_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_remoting_MODULES)
    message(WARNING
      "No modules were requested for remoting wrapping.")
    return ()
  endif ()

  if (NOT _vtk_remoting_TARGET)
    message(FATAL_ERROR
      "The `TARGET` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_remoting_DESTINATION)
    set(_vtk_remoting_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (NOT DEFINED _vtk_remoting_COMPONENT)
    set(_vtk_remoting_COMPONENT "development")
  endif ()

  if (NOT DEFINED _vtk_remoting_FUNCTION_NAME)
    set(_vtk_remoting_FUNCTION_NAME "${_vtk_remoting_TARGET}_initialize")
  endif ()

  # Disable CMake's automoc support for these targets.
  set(CMAKE_AUTOMOC 0)
  set(CMAKE_AUTORCC 0)
  set(CMAKE_AUTOUIC 0)

  # TODO: Install cmake properties?

  set(_vtk_remoting_all_modules)
  set(_vtk_remoting_all_wrapped_modules)
  foreach (_vtk_remoting_module IN LISTS _vtk_remoting_MODULES)
    _vtk_module_get_module_property("${_vtk_remoting_module}"
      PROPERTY  "exclude_wrap"
      VARIABLE  _vtk_remoting_exclude_wrap)
    if (_vtk_remoting_exclude_wrap)
      continue ()
    endif ()
    _vtk_module_get_module_property("${_vtk_remoting_module}"
      PROPERTY  "remoting_exclude"
      VARIABLE  _vtk_remoting_exclude)
    if (_vtk_remoting_exclude)
      continue ()
    endif ()
    _vtk_module_real_target(_vtk_remoting_target_name "${_vtk_remoting_module}")
    _vtk_module_get_module_property("${_vtk_remoting_module}"
      PROPERTY  "library_name"
      VARIABLE  _vtk_remoting_library_name)
    _vtk_module_wrap_remoting_library("${_vtk_remoting_library_name}RXX" "${_vtk_remoting_module}")

    if (TARGET "${_vtk_remoting_library_name}RXX")
      list(APPEND _vtk_remoting_all_modules
        "${_vtk_remoting_library_name}RXX")
      list(APPEND _vtk_remoting_all_wrapped_modules
        "${_vtk_remoting_module}")
    endif ()
  endforeach ()

  if (NOT _vtk_remoting_all_modules)
    message(FATAL_ERROR
      "No modules given could be wrapped.")
  endif ()

  if (DEFINED _vtk_remoting_WRAPPED_MODULES)
    set("${_vtk_remoting_WRAPPED_MODULES}"
      "${_vtk_remoting_all_wrapped_modules}"
      PARENT_SCOPE)
  endif ()

  if (_vtk_remoting_TARGET)
    add_library("${_vtk_remoting_TARGET}" INTERFACE)
    target_include_directories("${_vtk_remoting_TARGET}"
      INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_remoting_TARGET}>")
    set(_vtk_remoting_all_modules_include_file
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_remoting_TARGET}/${_vtk_remoting_TARGET}.h")

    set(_vtk_remoting_declarations)
    set(_vtk_remoting_calls)
    foreach (_vtk_remoting_module IN LISTS _vtk_remoting_all_modules)
      string(APPEND _vtk_remoting_declarations
        "extern \"C\" void ${_vtk_remoting_module}_Initialize();\n")
      string(APPEND _vtk_remoting_calls
        "  ${_vtk_remoting_module}_Initialize();\n")
    endforeach ()

    set(_vtk_remoting_all_modules_include_content
      "#ifndef ${_vtk_remoting_TARGET}_h
#define ${_vtk_remoting_TARGET}_h

${_vtk_remoting_declarations}
inline void ${_vtk_remoting_FUNCTION_NAME}()
{
${_vtk_remoting_calls}
}

#endif\n")

    file(GENERATE
      OUTPUT  "${_vtk_remoting_all_modules_include_file}"
      CONTENT "${_vtk_remoting_all_modules_include_content}")

    target_link_libraries("${_vtk_remoting_TARGET}"
      INTERFACE
        ${_vtk_remoting_all_modules})

    set(_vtk_remoting_export)
    if (_vtk_remoting_INSTALL_EXPORT)
      list(APPEND _vtk_remoting_export
        EXPORT "${_vtk_remoting_INSTALL_EXPORT}")
    endif ()

    install(
      TARGETS             "${_vtk_remoting_TARGET}"
      ${_vtk_remoting_export}
      COMPONENT           "${_vtk_remoting_COMPONENT}"
      ARCHIVE DESTINATION "${_vtk_remoting_DESTINATION}")
  endif ()
endfunction ()

#[==[.md
## Excluding a module from wrapping

Some modules should not be wrapped using remoting bindings. Since this is
independent of general wrapping facilities, an additional property is used to
check. This may be set using the `vtk_module_remoting_exclude` function.

```
vtk_module_remoting_exclude(
  [MODULE <module>])
```

The `MODULE` defaults to the module currently being built. If a module is not
being built when this function is called, it must be provided.
#]==]
function (vtk_module_remoting_exclude)
  cmake_parse_arguments(_vtk_remoting_exclude
    ""
    "MODULE"
    ""
    ${ARGN})

  if (_vtk_remoting_exclude_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_wrap_remoting_exclude: "
      "${_vtk_remoting_exclude_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_remoting_exclude_MODULE)
    if (NOT DEFINED _vtk_build_module)
      message(FATAL_ERROR
        "The `MODULE` argument must be provided outside of a module build.")
    endif ()
    set(_vtk_remoting_exclude_MODULE "${_vtk_build_module}")
  endif ()

  _vtk_module_set_module_property("${_vtk_remoting_exclude_MODULE}"
    PROPERTY  "remoting_exclude"
    VALUE     1)
endfunction ()

cmake_policy(POP)
