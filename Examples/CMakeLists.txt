cmake_minimum_required(VERSION 3.8)
project(ParaViewExamples)

if (ParaView_BINARY_DIR)
  function (add_example_impl dir mode paraview_root)
    cmake_parse_arguments(arg "" "TEST_TIMEOUT" "" ${ARGN})
    if (arg_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "Unknown arguments passed to 'add_example_impl': ${arg_UNPARSED_ARGUMENTS}")
    endif()

    set (build_dir "${CMAKE_CURRENT_BINARY_DIR}/${mode}/${dir}")
    set(tname "ParaViewExample-${mode}-${dir}")
    if (CMAKE_VERSION VERSION_LESS "3.17")
      add_test(
        NAME    "${tname}-prepare"
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${build_dir}")
    else()
      add_test(
        NAME    "${tname}-prepare"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${build_dir}")
    endif()
    add_test(
      NAME    "${tname}"
      COMMAND "${CMAKE_CTEST_COMMAND}"
              --build-generator
                "${CMAKE_GENERATOR}"
              --build-and-test
                "${CMAKE_CURRENT_SOURCE_DIR}/${dir}"
                "${build_dir}"
              --build-options
                "-DBUILD_TESTING:BOOL=ON"
                "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
                "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
                "-DParaView_DIR:PATH=${paraview_root}/${paraview_cmake_destination}"
              --test-command
                "${CMAKE_CTEST_COMMAND}"
                  -C $<CONFIGURATION>)
    # setup fixtures.
    set_tests_properties("${tname}-prepare"
      PROPERTIES
        FIXTURES_SETUP "${tname}-prepare")
    set(fixtures_required "${tname}-prepare")
    if (mode STREQUAL "install")
      # add a dependency of the ParaView::Install fixture.
      list(APPEND fixtures_required "ParaView::Install")
    endif()
    set_tests_properties("${tname}"
      PROPERTIES
        FIXTURES_REQUIRED "${fixtures_required}")

    if (WIN32)
      set(example_path "$ENV{PATH};${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
      string(REPLACE ";" "\;" example_path "${example_path}")
      set_property(TEST "${tname}" APPEND
        PROPERTY
          ENVIRONMENT "PATH=${example_path}")
    endif ()
    set_property(TEST "${tname}" APPEND
      PROPERTY
        SKIP_REGULAR_EXPRESSION "Skipping example")

    if (arg_TEST_TIMEOUT)
      # override timeout, if specified.
      set_property(TEST "${tname}"
        PROPERTY
          TIMEOUT "${arg_TEST_TIMEOUT}")
    endif()
  endfunction ()

  function(add_example dir)
    add_example_impl("${dir}" "build" "${ParaView_BINARY_DIR}" ${ARGN})
    if (PARAVIEW_INSTALL_DEVELOPMENT_FILES)
      # note, the path is defined in ParaViewTestInstall.cmake
      add_example_impl("${dir}" "install" "${CMAKE_BINARY_DIR}/Testing/Temporary/InstallRoot" ${ARGN})
    endif()
  endfunction()
else ()
  find_package(ParaView REQUIRED)

  macro (add_example dir)
    add_subdirectory("${dir}")
  endmacro ()
endif ()

add_example(Plugins/Autostart)
add_example(Plugins/DockWidget)
add_example(Plugins/ElevationFilter)
add_example(Plugins/GUIMyToolBar)
add_example(Plugins/LagrangianIntegrationModel)
add_example(Plugins/MyPNGReader)
add_example(Plugins/MyTiffWriter)
add_example(Plugins/OverrideXMLOnly)
add_example(Plugins/PropertyWidgets)
add_example(Plugins/ReaderXMLOnly)
add_example(Plugins/Representation)
# TODO: update this plugin to use the pipeline controller instead.
#add_example(Plugins/RepresentationBehavior)
add_example(Plugins/SMParametricSource)
add_example(Plugins/SMMyProxy)
add_example(Plugins/SourceToolbar)
# add_example(Plugins/VisItReader)

add_example(CustomApplications/Clone1)
add_example(CustomApplications/Clone2)
add_example(CustomApplications/Demo0)
add_example(CustomApplications/Demo1)
add_example(CustomApplications/MultiServerClient)
add_example(CustomApplications/SimpleParaView)
# TODO Update this custom application to load only specific data types.
# add_example(CustomApplications/Spreadsheet)

# increate timeout for this test since it builds multiple executables
# which be timeconsuming on static builds.
add_example(Catalyst
  TEST_TIMEOUT 600)

add_example(Catalyst2/CFullExample)
add_example(Catalyst2/CxxFullExample)
add_example(Catalyst2/CxxImageDataExample)
add_example(Catalyst2/CxxMultiChannelInputExample)
