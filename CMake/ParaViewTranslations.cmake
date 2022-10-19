#[==[
@brief Generate a C++ header with input XML labels that can be parsed by
Qt Linguist tools.

~~~
paraview_generate_translation_header(
  [TARGET        <name>]
  DIRECTORY      <directory>
  RESULT_FILE    <file>
  INPUT_FILES    <file>...)
~~~

  * `TARGET`: (Defaults to RESULT_FILE) An optional variable with the target name.
  * `DIRECTORY`: (Required) The source directory to search translatable strings in.
  * `RESULT_FILE`: (Required) The relative path of the desired result file.
  * `INPUT_FILES`: (Required) The relative path of the input files.
#]==]
function (paraview_generate_translation_header)
    cmake_parse_arguments(PARSE_ARGV 0 _pv_generate_tr_h
            ""
            "TARGET;DIRECTORY;RESULT_FILE"
            "INPUT_FILES")
    if (_pv_generate_tr_h_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
                "Unrecognized arguments for paraview_generate_translation_header: "
                "${_pv_generate_tr_h_UNPARSED_ARGUMENTS}.")
    endif ()
    if (NOT DEFINED _pv_generate_tr_h_RESULT_FILE)
        message(FATAL_ERROR
                "The `RESULT_FILE` argument is required.")
    endif ()
    if (NOT DEFINED _pv_generate_tr_h_TARGET)
        set(_pv_generate_tr_h_TARGET "${_pv_generate_tr_h_RESULT_FILE}")
    endif ()
    if (NOT DEFINED _pv_generate_tr_h_DIRECTORY)
        message(FATAL_ERROR
                "The `DIRECTORY` argument is required.")
    endif ()
    if (NOT DEFINED _pv_generate_tr_h_INPUT_FILES)
        message(FATAL_ERROR
                "The `INPUT_FILES` argument is required.")
    endif ()

    list(TRANSFORM _pv_generate_tr_h_INPUT_FILES PREPEND "${_pv_generate_tr_h_DIRECTORY}/")
    set(_pv_generate_tr_h_RESULT_FILE "${_pv_generate_tr_h_DIRECTORY}/${_pv_generate_tr_h_RESULT_FILE}")
    add_custom_target("${_pv_generate_tr_h_TARGET}" ALL
            COMMAND python ${CMAKE_SOURCE_DIR}/Utilities/Localization/parsing.py -o ${_pv_generate_tr_h_RESULT_FILE} ${_pv_generate_tr_h_INPUT_FILES}
            VERBATIM)
endfunction ()

#[==[
@brief Generate a Qt translation source file from the given source directory

~~~
paraview_create_translation(
  [RETURNED_QM   <file>]
  DIRECTORY      <directory>
  OUTPUT_TS      <file>
~~~

  * `RETURNED_QM`: An optional variable with the generated file path set as value.
  * `DIRECTORY`: (Required) The source directory to search translatable strings in.
  * `OUTPUT_TS`: (Required) The absolute path of the desired result file.
#]==]
function (paraview_create_translation)
    cmake_parse_arguments(PARSE_ARGV 0 _pv_create_tr
    ""
    "RETURNED_QM;DIRECTORY;OUTPUT_TS"
    "")
    if (_pv_create_tr_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
                "Unrecognized arguments for paraview_create_translation: "
                "${_pv_create_tr_UNPARSED_ARGUMENTS}.")
    endif ()
    if (NOT DEFINED _pv_create_tr_DIRECTORY)
        message(FATAL_ERROR
                "The `DIRECTORY` argument is required.")
    endif ()
    if (NOT DEFINED _pv_create_tr_OUTPUT_TS)
        message(FATAL_ERROR
                "The `OUTPUT_TS` argument is required.")
    endif ()

    find_package(Qt5 REQUIRED QUIET COMPONENTS LinguistTools)
    get_filename_component(_dir_ts ${_pv_create_tr_OUTPUT_TS} DIRECTORY)
    set_source_files_properties("${_pv_create_tr_OUTPUT_TS}" PROPERTIES OUTPUT_LOCATION "${_dir_ts}")
    qt5_create_translation(result_qm "${_pv_create_tr_DIRECTORY}" "${_pv_create_tr_OUTPUT_TS}")
    if (DEFINED _pv_create_tr_RETURNED_QM)
        set(${_pv_create_tr_RETURNED_QM} ${result_qm} PARENT_SCOPE)
    endif()
    get_property(_create_translation_tmp_qm_list GLOBAL PROPERTY paraview_qm_files)
    list(APPEND _create_translation_tmp_qm_list "${result_qm}")
    set_property(GLOBAL PROPERTY paraview_qm_files "${_create_translation_tmp_qm_list}")
endfunction ()
