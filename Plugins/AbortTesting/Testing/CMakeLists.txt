ExternalData_Expand_Arguments("ParaViewData" _
  "DATA{Data/unbalanced.vtkhdf}"
)

# Add AbortPipeline tests
paraview_add_client_tests(
  TEST_SCRIPTS AbortPipeline.xml
)
paraview_add_client_server_tests(
  TEST_SCRIPTS AbortPipelineCS.xml
  NUMSERVERS 3
)
paraview_add_client_server_render_tests(
  TEST_SCRIPTS AbortPipelineCS.xml
  NUMSERVERS 3
)

paraview_add_client_server_tests(
  TEST_SCRIPTS AbortPipelineCSUnbalanced.xml
  NUMSERVERS 3
)
