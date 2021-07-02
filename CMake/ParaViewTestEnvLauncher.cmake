cmake_minimum_required(VERSION 3.12)

if (WIN32 AND extra_dll_paths)
  # Append extra directories to PATH as necessary.
  set(ENV{PATH}
    "$ENV{PATH};${extra_dll_paths}")
endif ()

execute_process(
  COMMAND
    ${test_command}
  RESULT_VARIABLE res)

if (res)
  message(FATAL_ERROR "Test command failed")
endif ()
