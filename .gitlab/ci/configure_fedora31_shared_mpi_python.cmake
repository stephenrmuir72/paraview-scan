set(MPI_PREFLAGS                        "--mca orte_base_help_aggregate 0"  CACHE STRING "")
#set(PARAVIEW_MULTI_CLIENT_TESTING ON CACHE STRING "")
set(PARAVIEW_USE_QT ON CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora31.cmake")
