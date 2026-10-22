if (BUILD_SHARED_LIBS)

  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/Baseline/SaveStateAndScreenshot.png}"
    "DATA{Data/Baseline/SaveStateAndScreenshotActiveView.png}"
    "DATA{Data/Baseline/SaveStateAndScreenshotActiveView2.png}")

  set(test_scripts
    "${CMAKE_CURRENT_SOURCE_DIR}/SaveStateAndScreenshot.xml")
  # On macOS HiDPI displays, the view size is doubled so the screenshot
  # does not match the baseline size.
  if (NOT APPLE)
    list(APPEND test_scripts
      "${CMAKE_CURRENT_SOURCE_DIR}/SaveStateAndScreenshotActiveView.xml")
  endif ()

  paraview_add_client_tests(
    LOAD_PLUGIN "SaveStateAndScreenshot"
    TEST_SCRIPTS ${test_scripts})
endif()
