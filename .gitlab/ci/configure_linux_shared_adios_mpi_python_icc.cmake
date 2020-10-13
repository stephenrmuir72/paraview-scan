set(CMAKE_PREFIX_PATH                   "/usr/lib64/openmpi" CACHE STRING "")
set(PARAVIEW_BUILD_SHARED_LIBS          ON                   CACHE BOOL "")
set(CMAKE_C_COMPILER                    "icc"                CACHE STRING "")
set(CMAKE_CXX_COMPILER                  "icpc"               CACHE STRING "")

# Using OSMESA as we cannot get to work a native newer openGL version with
# Docker in Gitlab CI. :TODO:
#set(VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN ON          CACHE BOOL "")
#set(VTK_OPENGL_HAS_OSMESA               ON          CACHE BOOL "")
set(VTK_USE_X                           ON         CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
