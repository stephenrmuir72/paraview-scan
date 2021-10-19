# These tests depend on target ParaView::RemotingMisc, which may not
# exist in the edition being built. Exit
if (NOT TARGET ParaView::RemotingMisc)
  return()
endif ()

ExternalData_Expand_Arguments(ParaViewData _
  # data files
  "DATA{${paraview_test_data_directory_input}/Data/globbing/glob_a.png}"
  "DATA{${paraview_test_data_directory_input}/Data/globbing/glob_b.png}"
  "DATA{${paraview_test_data_directory_input}/Data/globbing/glob_c.png}"
  "DATA{${paraview_test_data_directory_input}/Data/globbing/glob_d.png}"
  "DATA{${paraview_test_data_directory_input}/Data/globbing/foo_a.png}"
  "DATA{${paraview_test_data_directory_input}/Data/globbing/foo_b.png}"
  )

paraview_add_test_python(
  NO_VALID
  Settings.py
  TestHelperProxySerialization.py
  TestMultiplexerSourceProxy.py
  TestReaderGlobbing.py
  )
