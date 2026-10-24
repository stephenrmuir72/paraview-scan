set(xml_files SceneGraph_SM.xml)

set(sources)
set(interfaces)
set(ui_files)


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

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
