set(xml_files SceneGraph_SM.xml)

set(sources)
set(interfaces)
set(ui_files)

if (PARAVIEW_USE_QT)
  list(APPEND sources
    pqSceneGraphDockPanel.cxx
    pqSceneGraphDockPanel.h
    pqSceneGraphModel.cxx
    pqSceneGraphModel.h)
  list(APPEND ui_files
    pqSceneGraphDockPanel.ui)

  paraview_plugin_add_dock_window(
    CLASS_NAME pqSceneGraphDockPanel
    DOCK_AREA Left
    INTERFACES dock_interfaces
    SOURCES dock_sources)
  list(APPEND interfaces
    ${dock_interfaces})
  list(APPEND sources
    ${dock_sources})
endif ()

paraview_add_plugin(SceneGraph
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  UI_FILES ${ui_files}
  SOURCES ${sources}
  SERVER_MANAGER_XML ${xml_files}
  MODULES ParaViewIncubator::SceneGraph
  XML_DOCUMENTATION OFF)

target_link_libraries(SceneGraph
  PRIVATE
    ParaView::RemotingServerManager
    ParaView::RemotingViews
    VTK::CommonCore
    VTK::CommonMath)

if (PARAVIEW_USE_QT)
  target_compile_definitions(SceneGraph PRIVATE QT_NO_KEYWORDS)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
