# Catalyst V2 tests driven from Python.
#
# These drive the Catalyst V2 conduit ABI directly through the catalyst SDK
# (the mini-app calls catalyst.execute()), for channels the in-process Catalyst
# bridge used by paraview.demos.wavelet_miniapp cannot exercise. That means the
# test process needs the catalyst + catalyst_conduit + conduit Python modules on
# PYTHONPATH and needs to be able to locate the ParaView Catalyst
# implementation.

# Base conduit Python module (needed by catalyst_conduit).
get_target_property(_catalyst_conduit_py_dir VTK::conduit_python VTK_CONDUIT_PYTHON_MODULE_DIR)

# catalyst + catalyst_conduit Python modules, from the catalyst SDK.
find_package(catalyst 2.0 QUIET)
set(_catalyst_py_dir)
if (CATALYST_PYTHONPATH)
  # CATALYST_PYTHONPATH may be a path-list; the first entry is the SDK's
  # site-packages holding the catalyst and catalyst_conduit packages.
  string(REPLACE ":" ";" _catalyst_pp "${CATALYST_PYTHONPATH}")
  list(GET _catalyst_pp 0 _catalyst_py_dir)
endif ()

if (NOT _catalyst_conduit_py_dir OR NOT _catalyst_py_dir OR NOT TARGET catalyst-paraview)
  message(STATUS
    "Skipping Catalyst Python tests: need conduit Python "
    "(${_catalyst_conduit_py_dir}), catalyst Python (${_catalyst_py_dir}) "
    "and the catalyst-paraview implementation.")
  return ()
endif ()

set(_catalyst_test_dir "${CMAKE_CURRENT_SOURCE_DIR}")

# Runs MINIAPP with pvbatch and the Catalyst SCRIPT, on RANKS ranks if given.
function (_catalyst_python_test)
  cmake_parse_arguments(PARSE_ARGV 0 _test "" "NAME;MINIAPP;SCRIPT;RANKS" "ARGS")
  set(_pvbatch "$<TARGET_FILE:ParaView::pvbatch>" --dr --sym)
  if (_test_RANKS)
    set(_pvbatch
      "${MPIEXEC_EXECUTABLE}" ${MPIEXEC_NUMPROC_FLAG} ${_test_RANKS} ${MPIEXEC_PREFLAGS}
      "$<TARGET_FILE:ParaView::pvbatch>" --mpi --dr --sym ${MPIEXEC_POSTFLAGS})
  endif ()
  set(_command ${_pvbatch}
    "${_catalyst_test_dir}/${_test_MINIAPP}" ${_test_ARGS}
    -s "${_catalyst_test_dir}/${_test_SCRIPT}")
  add_test(NAME "${_test_NAME}" COMMAND ${_command})
  set_tests_properties("${_test_NAME}"
    PROPERTIES
      ENVIRONMENT "CATALYST_IMPLEMENTATION_PATHS=$<TARGET_FILE_DIR:catalyst-paraview>")
  set_property(TEST "${_test_NAME}" APPEND PROPERTY
    ENVIRONMENT_MODIFICATION
      "PYTHONPATH=path_list_append:${_catalyst_py_dir}"
      "PYTHONPATH=path_list_append:${_catalyst_conduit_py_dir}")

  # The CI image has multiple libcatalyst with the same soname (one per MPI
  # flavor under /usr and /usr/lib64/<mpi> using Catalyst's mangled Conduit,
  # plus one per MPI flavor under /opt/catalyst-ext using the external Conduit
  # installs. Make sure these tests load the one that matches the how ParaView
  # was built.
  if (UNIX AND NOT APPLE)
    set_property(TEST "${_test_NAME}" APPEND PROPERTY
      ENVIRONMENT_MODIFICATION
        "LD_LIBRARY_PATH=path_list_prepend:$<TARGET_FILE_DIR:catalyst::catalyst>")

    # Print a warning for older cmake versions
    if (CMAKE_VERSION VERSION_LESS "3.22")
      message(WARNING
        "CMake 3.22+ is required to reliably set up the environment for tests "
        "needing to load a specific Catalyst. ${_test_NAME} may fail to load "
        "the correct catalyst and fail as a result.")
    endif ()
  endif ()
endfunction ()

foreach (_case IN ITEMS unstructured cellgrid)
  _catalyst_python_test(
    NAME "ParaView::Catalyst::FidesConduit::${_case}"
    MINIAPP fides_catalyst_miniapp.py
    SCRIPT "fides_conduit_${_case}.py"
    ARGS --grid "${_case}")
  set_tests_properties("ParaView::Catalyst::FidesConduit::${_case}"
    PROPERTIES
      PASS_REGULAR_EXPRESSION "All ok")
endforeach ()

# Ranks without data for a channel pass an empty node, or leave the channel out, which must fail.
if (PARAVIEW_USE_MPI AND MPIEXEC_EXECUTABLE)
  _catalyst_python_test(
    NAME ParaView::Catalyst::EmptyChannel
    MINIAPP missing_channel_miniapp.py
    SCRIPT missing_channel_pipeline.py
    RANKS 3)
  _catalyst_python_test(
    NAME ParaView::Catalyst::MissingChannel
    MINIAPP missing_channel_miniapp.py
    SCRIPT missing_channel_pipeline.py
    RANKS 3
    ARGS --missing)
  _catalyst_python_test(
    NAME ParaView::Catalyst::EmptyFidesChannel
    MINIAPP missing_channel_miniapp.py
    SCRIPT missing_channel_pipeline.py
    RANKS 3
    ARGS --fides)
  set_tests_properties(ParaView::Catalyst::EmptyChannel ParaView::Catalyst::EmptyFidesChannel
    PROPERTIES TIMEOUT 120)
  set_tests_properties(ParaView::Catalyst::MissingChannel
    PROPERTIES
      TIMEOUT 120
      PASS_REGULAR_EXPRESSION "ranks pass different channels")
endif ()
