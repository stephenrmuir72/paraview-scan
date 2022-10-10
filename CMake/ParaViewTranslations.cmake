function (merge_all_qm)
    cmake_parse_arguments(_merge_args
    ""
    "PATH"
    ""
    ${ARGN})
    if (NOT DEFINED _merge_args_PATH)
        set(_merge_args_PATH "${CMAKE_CURRENT_BINARY_DIR}")
    endif ()
    get_property(_merge_args_qm_files GLOBAL PROPERTY paraview_qm_files)

    add_custom_target("lang_fr.qm" COMMAND lconvert -o lang_fr.qm ${_merge_args_qm_files} VERBATIM)
    message(STATUS "lconvert -o lang_fr.qm" ${_merge_args_qm_files})
endfunction ()

function (paraview_create_translation OUTPUT_QM output_qm DIRECTORY dir_to_scan OUTPUT_TS output_ts_dir)
    find_package(Qt5 REQUIRED QUIET COMPONENTS LinguistTools)
    get_filename_component(_dir_ts ${output_ts_dir} DIRECTORY)
    set_source_files_properties("${output_ts_dir}" PROPERTIES OUTPUT_LOCATION "${_dir_ts}")
    qt5_create_translation(result_qm "${CMAKE_CURRENT_SOURCE_DIR}" "${output_ts_dir}")
    set(${output_qm} ${result_qm} PARENT_SCOPE)
    get_property(_create_translation_tmp_qm_list GLOBAL PROPERTY paraview_qm_files)
    list(APPEND _create_translation_tmp_qm_list "${result_qm}")
    set_property(GLOBAL PROPERTY paraview_qm_files "${_create_translation_tmp_qm_list}")
endfunction ()
