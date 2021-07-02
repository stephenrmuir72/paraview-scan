cmake_minimum_required(VERSION 3.12)

execute_process(
  COMMAND
    ${test_command}
  RESULT_VARIABLE res)

if (res)
  message(FATAL_ERROR "Test command failed")
endif ()
