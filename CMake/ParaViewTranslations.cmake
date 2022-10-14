#[==[
@brief Generate a Qt translation binary file from translation source files

~~~
merge_all_qm(
  FILE      <file>
~~~

  * `FILE`: (Required) The absolute path of the file to generate.
#]==]
function (merge_all_qm)
    cmake_parse_arguments(PARSE_ARGV 0 _merge_args
    ""
    "FILE"
    "")
    if (_merge_args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
                "Unrecognized arguments for merge_all_qm: "
                "${_merge_args_UNPARSED_ARGUMENTS}.")
    endif ()
    if (NOT DEFINED _merge_args_FILE)
        message(FATAL_ERROR
                "The `FILE` argument is required.")
    endif ()
    get_property(_merge_args_qm_files GLOBAL PROPERTY paraview_qm_files)

    add_custom_target("lang_fr.qm" ALL COMMAND lconvert -o ${_merge_args_FILE} ${_merge_args_qm_files} VERBATIM)
    add_dependencies("lang_fr.qm" paraview)
    install(FILES "${_merge_args_FILE}" DESTINATION bin)
    message(STATUS "lconvert -o lang_fr.qm" ${_merge_args_qm_files})
endfunction ()

#[==[
@brief Generate a Qt translation source file from the given source directory

Add a rule to turn a file into a MD5 hash and place that in a C string.

~~~
paraview_create_translation(
  [OUTPUT_QM      <file>]
  DIRECTORY      <directory>
  OUTPUT_TS      <file>
~~~

  * `OUTPUT_QM`: An optional variable with the generated file path set as value.
  * `DIRECTORY`: (Required) The source directory to search translatable strings in.
  * `OUTPUT_TS`: (Required) The absolute path of the desired result file.
#]==]
function (paraview_create_translation)
    cmake_parse_arguments(PARSE_ARGV 0 _pv_create_tr
    ""
    "OUTPUT_QM;DIRECTORY;OUTPUT_TS"
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
    if (DEFINED _pv_create_tr_OUTPUT_QM)
        set(${_pv_create_tr_OUTPUT_QM} ${result_qm} PARENT_SCOPE)
    endif()
    get_property(_create_translation_tmp_qm_list GLOBAL PROPERTY paraview_qm_files)
    list(APPEND _create_translation_tmp_qm_list "${result_qm}")
    set_property(GLOBAL PROPERTY paraview_qm_files "${_create_translation_tmp_qm_list}")
endfunction ()
