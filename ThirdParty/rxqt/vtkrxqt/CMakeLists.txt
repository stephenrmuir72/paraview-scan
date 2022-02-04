set(headers
  include/rxqt.hpp
  include/rxqt_event.hpp
  include/rxqt_run_loop.hpp
  include/rxqt_run_loop_thread.hpp
  include/rxqt_signal.hpp
  include/rxqt_util.hpp
  include/vtkrxqt.hpp)

vtk_module_find_package(
  PACKAGE     "Qt${VTK_QT_VERSION}"
  COMPONENTS  Core
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         "Qt${VTK_QT_VERSION}_VERSION")

vtk_module_add_module(ParaView::rxqt
  HEADER_ONLY HEADER_DIRECTORIES
  HEADERS ${headers}
  HEADER_SUBDIR "rxqt/include")
vtk_module_link(ParaView::rxqt
  INTERFACE
    "Qt${VTK_QT_VERSION}::Core")
