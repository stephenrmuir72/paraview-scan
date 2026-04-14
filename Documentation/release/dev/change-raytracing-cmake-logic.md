## Improve Ray Tracing integration

### Improve Ray Tracing capabilities display in UI

Checkboxes named **Enable Ray Tracing** and **Enable Rendering with ANARI** are now shown only if ParaView is shipped with it.

### Modify CMake default logic for Ray Tracing related modules

`PARAVIEW_ENABLE_RAYTRACING` CMake variable is now automatically turned `ON` or `OFF` depending on OSPRay or ANARI. One should now enable either `VTK_ENABLE_OSPRAY` or `PARAVIEW_ENABLE_ANARI` to activate their respective module. Note that both module can be activated at the same time.
