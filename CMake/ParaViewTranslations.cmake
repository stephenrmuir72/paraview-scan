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
    if (NOT DEFINED _pv_create_tr_OUTPUT_QM)
        message(FATAL_ERROR
                "The `OUTPUT_QM` argument is required.")
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
    set(${_pv_create_tr_OUTPUT_QM} ${result_qm} PARENT_SCOPE)
    get_property(_create_translation_tmp_qm_list GLOBAL PROPERTY paraview_qm_files)
    list(APPEND _create_translation_tmp_qm_list "${result_qm}")
    set_property(GLOBAL PROPERTY paraview_qm_files "${_create_translation_tmp_qm_list}")
endfunction ()
