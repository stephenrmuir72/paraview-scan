# ParaViewTesting environment modification

The `ParaViewTesting` module APIs which execute a ParaView client now support
the `ADDITIONAL_DLL_PATHS` to add directories to `PATH` on Windows when
executing the test. The following APIs support this new argument:

  - `paraview_add_client_tests`
  - `paraview_add_client_server_tests`
  - `paraview_add_client_server_render_tests`
  - `paraview_add_multi_server_tests count`
  - `paraview_add_tile_display_tests width height`
  - `paraview_add_cave_tests num_ranks config`
  - `paraview_add_test`
  - `paraview_add_test_mpi`
  - `paraview_add_test_mpi_optional`
