# OmegaHReader plugin XML tests: open an omega_h mesh through the GUI and
# compare the rendered image with a baseline.
paraview_add_client_tests(
  LOAD_PLUGIN "OmegaHReader"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS OmegaHReaderTest.xml)

paraview_add_client_server_tests(
  LOAD_PLUGIN "OmegaHReader"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS OmegaHReaderTest.xml)
