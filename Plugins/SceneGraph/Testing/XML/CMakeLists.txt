# While this test does not need data or baseline images, TEST_DATA_TARGET is
# required by the macro
paraview_add_client_tests(
  LOAD_PLUGIN "SceneGraph"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS TestSceneGraphPanel.xml)
