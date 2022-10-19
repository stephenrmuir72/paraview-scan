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
