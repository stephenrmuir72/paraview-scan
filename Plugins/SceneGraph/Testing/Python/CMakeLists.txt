set(_vtk_build_test "paraview")
set(${_vtk_build_test}_TEST_LABELS paraview)

set(py_tests
  TestSceneGraphScaffold.py,NO_DATA,NO_VALID,NO_OUTPUT
  TestSceneGraphTree.py,NO_DATA,NO_VALID,NO_OUTPUT
  TestSceneGraphTransform.py,NO_DATA,NO_VALID,NO_OUTPUT
  TestSceneGraphBinding.py,NO_DATA,NO_VALID,NO_OUTPUT
  TestSceneGraphInvariants.py,NO_DATA,NO_VALID,NO_OUTPUT
  TestSceneGraphErrorCodes.py,NO_DATA,NO_VALID,NO_OUTPUT
  TestSceneGraphWrapper.py,NO_DATA,NO_VALID,NO_OUTPUT
  TestSceneGraphInteractors.py,NO_DATA,NO_VALID,NO_OUTPUT)

paraview_add_test_python(${py_tests})

# The state test is one script run twice, in separate processes, one to save a
# state file, another to load it.
set(_statefile "${PARAVIEW_TEST_DIR}/SceneGraphState.pvsm")

set(TestSceneGraphState_ARGS save "${_statefile}")
set(vtk_test_prefix "Save-")
paraview_add_test_python(TestSceneGraphState.py,NO_DATA,NO_VALID,NO_OUTPUT)

set(TestSceneGraphState_ARGS load "${_statefile}")
set(vtk_test_prefix "Load-")
paraview_add_test_python(TestSceneGraphState.py,NO_DATA,NO_VALID,NO_OUTPUT)

# Make the save-state test a fixture the load-state test can depend on
set(_prefix "${_vtk_build_test}Python-")
set_tests_properties("${_prefix}Save-TestSceneGraphState"
  PROPERTIES FIXTURES_SETUP SceneGraphState)
set_tests_properties("${_prefix}Load-TestSceneGraphState"
  PROPERTIES FIXTURES_REQUIRED SceneGraphState)
