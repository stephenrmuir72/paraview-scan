# Set variables to make the testing functions.
set(_vtk_build_test "paraview")
set(${_vtk_build_test}_TEST_LABELS paraview)

paraview_add_test_python(
  NO_RT
  OmegaHReaderReadMeshes.py
)
