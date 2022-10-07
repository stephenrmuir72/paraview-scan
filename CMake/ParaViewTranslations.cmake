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
    "${_merge_args_PATH}/Qt/ApplicationComponents/lang_fr.qm"
    "${_merge_args_PATH}/Qt/Components/lang_fr.qm"
    "${_merge_args_PATH}/Qt/Core/lang_fr.qm"
    "${_merge_args_PATH}/Qt/Python/lang_fr.qm"
    "${_merge_args_PATH}/Qt/Widgets/lang_fr.qm"
    "${_merge_args_PATH}/Clients/ParaView/lang_fr.qm"
    "${_merge_args_PATH}/Clients/ParaView/ParaViewClient_fr.qm"
    )

    set (lconvert_cmd "lconvert")
    set (lconvert_arg 
    "-o"
    "lang_fr.qm"
    "${qm_files}"
    )
    execute_process(COMMAND ${lconvert_cmd} ${lconvert_arg}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endfunction ()

function (paraview_create_translation OUTPUT_QM output_qm DIRECTORY dir_to_scan OUTPUT_TS output_ts)
    find_package(Qt5 REQUIRED QUIET COMPONENTS LinguistTools)
    get_filename_component(_dir_ts ${output_ts} DIRECTORY)
    set_source_files_properties("${output_ts}" PROPERTIES OUTPUT_LOCATION "${_dir_ts}")
    qt5_create_translation(output_qm "${CMAKE_CURRENT_SOURCE_DIR}" "${output_ts}")
    set(${_qm_files} ${output_qm} PARENT_SCOPE)
endfunction ()
