function (merge_all_qm)
    cmake_parse_arguments(_merge_args
    ""
    "PATH"
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
