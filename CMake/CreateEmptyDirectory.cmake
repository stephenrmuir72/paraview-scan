# Deletes an old directory and creates a new one in its place.

if (NOT DEFINED TARGET_DIRECTORY)
  message(FATAL_ERROR "Missing 'TARGET_DIRECTORY'")
endif()

file(REMOVE_RECURSE "${TARGET_DIRECTORY}")
file(MAKE_DIRECTORY "${TARGET_DIRECTORY}")
