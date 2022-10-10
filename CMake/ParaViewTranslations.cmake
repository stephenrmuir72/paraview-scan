function (merge_all_qm)
    cmake_parse_arguments(_merge_args
    ""
    "PATH"
    ""
    ${ARGN})
    if (NOT DEFINED _merge_args_PATH)
        set(_merge_args_PATH "${CMAKE_CURRENT_BINARY_DIR}")
    endif ()
    set(qm_files
    "${_merge_args_PATH}/ApplicationComponents.qm"
    "${_merge_args_PATH}/Components.qm"
    "${_merge_args_PATH}/Core.qm"
    "${_merge_args_PATH}/Python.qm"
    "${_merge_args_PATH}/Widgets.qm"
    "${_merge_args_PATH}/Client.qm"
    )

    add_custom_target("lang_fr.qm" COMMAND lconvert -o lang_fr.qm ${qm_files} VERBATIM)
    message(STATUS "lconvert -o lang_fr.qm" ${qm_files})
endfunction ()

function (paraview_create_translation OUTPUT_QM output_qm DIRECTORY dir_to_scan OUTPUT_TS output_ts_dir)
    find_package(Qt5 REQUIRED QUIET COMPONENTS LinguistTools)
    get_filename_component(_dir_ts ${output_ts_dir} DIRECTORY)
    set_source_files_properties("${output_ts_dir}" PROPERTIES OUTPUT_LOCATION "${_dir_ts}")
    qt5_create_translation(output_qm "${CMAKE_CURRENT_SOURCE_DIR}" "${output_ts_dir}")
    set(${_qm_files} ${output_qm} PARENT_SCOPE)
endfunction ()
