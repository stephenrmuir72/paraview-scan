#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
set(classes
  pqAboutDialogReaction
  pqAddToFavoritesReaction
  pqAnimationShortcutDecorator
  pqAnimationShortcutWidget
  pqAnnotationsModel
  pqApplicationComponentsInit
  pqApplicationSettingsReaction
  pqApplyPropertiesReaction
  pqBackgroundEditorWidget
  pqBoolPropertyWidgetDecorator
  pqCameraManipulatorWidget
  pqCameraReaction
  pqCameraToolbar
  pqCatalystConnectReaction
  pqCatalystExportReaction
  pqCatalystPauseSimulationReaction
  pqCatalystRemoveBreakpointReaction
  pqCatalystSetBreakpointReaction
  pqCategoryToolbarsBehavior
  pqCheckableProperty
  pqChooseColorPresetReaction
  pqColorAnnotationsPropertyWidget
  pqColorAnnotationsWidget
  pqColorArrayPropertyWidget
  pqColorEditorPropertyWidget
  pqColorPaletteSelectorWidget
  pqColorSelectorPropertyWidget
  pqCTHArraySelectionDecorator
  pqDataAssemblyPropertyWidget
  pqDefaultViewBehavior
  pqDesktopServicesReaction
  pqDoubleRangeSliderPropertyWidget
  pqEditColorMapReaction
  pqEditScalarBarReaction
  pqEnableWidgetDecorator
  pqExampleVisualizationsDialog
  pqExampleVisualizationsDialogReaction
  pqFileListPropertyWidget
  pqFileNamePropertyWidget
  pqFiltersMenuReaction
  pqFontPropertyWidget
  pqGenericPropertyWidgetDecorator
  pqGlyphScaleFactorPropertyWidget
  pqImageCompressorWidget
  pqInputDataTypeDecorator
  pqListPropertyWidget
  pqLoadDataReaction
  pqLoadMaterialsReaction
  pqLoadStateReaction
  pqLockPanelsBehavior
  pqLockPanelsReaction
  pqManageFavoritesReaction
  pqMasterOnlyReaction
  pqMenuReactionUtils
  pqMoleculePropertyWidget
  pqOSPRayHidingDecorator
  pqParaViewBehaviors
  pqParaViewMenuBuilders
  pqProxyGroupMenuManager
  pqReaction
  pqRenameProxyReaction
  pqSaveStateReaction
  pqScalarBarVisibilityReaction
  pqSelectionQueryPropertyWidget
  pqServerConnectReaction
  pqServerDisconnectReaction
  pqSessionTypeDecorator
  pqShowWidgetDecorator
  pqSourcesMenuReaction
  pqStandardPropertyWidgetInterface
  pqStandardViewFrameActionsImplementation
  pqStandardRecentlyUsedResourceLoaderImplementation
  pqStatusBar
  pqTextLocationWidget
  pqUsageLoggingBehavior
  pqUseSeparateColorMapReaction
  pqViewMenuManager
  pqViewResolutionPropertyWidget
  pqViewTypePropertyWidget
  pqWelcomeDialog
  pqYoungsMaterialPropertyWidget
)

set(old_classes
  pqAnimationTimeToolbar
  pqAutoApplyReaction
  pqAutoLoadPluginXMLBehavior
  pqAxesToolbar
  pqBlockContextMenu
  pqBoxPropertyWidget
  pqCameraLinkReaction
  pqCameraUndoRedoReaction
  pqChangePipelineInputReaction
  pqChartSelectionReaction
  pqCollaborationBehavior
  pqColorOpacityEditorWidget
  pqColorMapEditor
  pqColorTableModel
  pqColorToolbar
  pqCommandButtonPropertyWidget
  pqCommandLineOptionsBehavior
  pqCopyReaction
  pqCrashRecoveryBehavior
  pqCreateCustomFilterReaction
  pqCustomShortcutBehavior
  pqCustomizeShortcutsReaction
  pqCustomizeShortcutsDialog
  pqCoordinateFramePropertyWidget
  pqCustomViewpointsToolbar
  pqCylinderPropertyWidget
  pqDataQueryReaction
  pqDataTimeStepBehavior
  pqDefaultContextMenu
  pqDeleteReaction
  pqDisplaySizedImplicitPlanePropertyWidget
  pqEditCameraReaction
  pqExportReaction
  pqExtractorsMenuReaction
  pqFindDataWidget
  pqHandlePropertyWidget
  pqIgnoreSourceTimeReaction
  pqImplicitPlanePropertyWidget
  pqInteractivePropertyWidget
  pqLightPropertyWidget
  pqLinePropertyWidget
  pqLinkSelectionReaction
  pqLiveSourceBehavior
  pqLoadPaletteReaction
  pqLoadRestoreWindowLayoutReaction
  pqLogViewerReaction
  pqMainControlsToolbar
  pqMainWindowEventBehavior
  pqManageCustomFiltersReaction
  pqManageLinksReaction
  pqManagePluginsReaction
  pqModelTransformSupportBehavior
  pqMultiComponentsDecorator
  pqObjectPickingBehavior
  pqOMETransferFunctionsPropertyWidget
  pqOpacityTableModel
  pqPVAnimationWidget
  pqPVApplicationCore
  pqPauseLiveSourcePropertyWidget
  pqPipelineContextMenuBehavior
  pqPluginActionGroupBehavior
  pqPluginDockWidgetsBehavior
  pqPluginSettingsBehavior
  pqPluginToolBarBehavior
  pqPointPickingHelper
  pqPreviewMenuManager
  pqPropertyCollectionWidget
  pqReloadFilesReaction
  pqRenderViewSelectionReaction
  pqRepresentationToolbar
  pqResetScalarRangeReaction
  pqResetDefaultSettingsReaction
  pqSaveAnimationGeometryReaction
  pqAnimatedExportReaction
  pqSaveAnimationReaction
  pqSaveDataReaction
  pqSaveExtractsReaction
  pqSaveScreenshotReaction
  pqSearchItemReaction
  pqSelectionReaction
  pqSeriesEditorPropertyWidget
  pqSetMainWindowTitleReaction
  pqShowHideAllReaction
  pqSpherePropertyWidget
  pqSplinePropertyWidget
  pqSpreadSheetViewDecorator
  pqSpreadSheetVisibilityBehavior
  pqStreamingTestingEventPlayer
  pqTemporalExportReaction
  pqTestingReaction
  pqTimeInspectorWidget
  pqTimerLogReaction
  pqToggleInteractionViewMode
  pqTransferFunctionWidgetPropertyDialog
  pqTransferFunctionWidgetPropertyWidget
  pqUndoRedoBehavior
  pqUndoRedoReaction
  pqVCRToolbar
  pqVerifyRequiredPluginBehavior
  pqViewStreamingBehavior
)

set_property(SOURCE pqApplicationComponentsInit.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "BUILD_SHARED_LIBS=$<BOOL:${BUILD_SHARED_LIBS}>")

set(headers
  pqCatalystContinueReaction.h
  )

if (TARGET ParaView::pqPython)
  list(APPEND classes
    pqMacroReaction
    pqTraceReaction
  )
  list(APPEND headers
    pqPythonScriptEditorReaction.h
  )
endif ()

set(ui_files
  Resources/UI/pqAxesToolbar.ui
  Resources/UI/pqBackgroundEditorWidget.ui
  Resources/UI/pqBoxPropertyWidget.ui
  Resources/UI/pqCameraManipulatorWidget.ui
  Resources/UI/pqCameraToolbar.ui
  Resources/UI/pqColorAnnotationsWidget.ui
  Resources/UI/pqColorArrayPropertyWidget.ui
  Resources/UI/pqColorEditorPropertyWidget.ui
  Resources/UI/pqColorMapEditor.ui
  Resources/UI/pqColorOpacityEditorWidget.ui
  Resources/UI/pqColorToolbar.ui
  Resources/UI/pqCustomResolutionDialog.ui
  Resources/UI/pqCylinderPropertyWidget.ui
  Resources/UI/pqCustomizeShortcutsDialog.ui
  Resources/UI/pqDataAssemblyPropertyWidget.ui
  Resources/UI/pqDefaultMainWindow.ui
  Resources/UI/pqDoubleRangeSliderPropertyWidget.ui
  Resources/UI/pqEditMenuBuilder.ui
  Resources/UI/pqEmptyView.ui
  Resources/UI/pqExampleVisualizationsDialog.ui
  Resources/UI/pqFileMenuBuilder.ui
  Resources/UI/pqFileListPropertyWidget.ui
  Resources/UI/pqFindDataWidget.ui
  Resources/UI/pqFontPropertyWidget.ui
  Resources/UI/pqHandlePropertyWidget.ui
  Resources/UI/pqImageCompressorWidget.ui
  Resources/UI/pqImplicitPlanePropertyWidget.ui
  Resources/UI/pqLightPropertyWidget.ui
  Resources/UI/pqLinePropertyWidget.ui
  Resources/UI/pqMainControlsToolbar.ui
  Resources/UI/pqMoleculePropertyWidget.ui
  Resources/UI/pqOMETransferFunctionsPropertyWidget.ui
  Resources/UI/pqOMETransferFunctionsPropertyWidgetPage.ui
  Resources/UI/pqPropertyCollectionWidget.ui
  Resources/UI/pqResetScalarRangeToDataOverTime.ui
  Resources/UI/pqSavePresetOptions.ui
  Resources/UI/pqSeriesEditorPropertyWidget.ui
  Resources/UI/pqSpherePropertyWidget.ui
  Resources/UI/pqSplinePropertyWidget.ui
  Resources/UI/pqSpreadSheetViewDecorator.ui
  Resources/UI/pqTextLocationWidget.ui
  Resources/UI/pqTimeInspectorWidget.ui
  Resources/UI/pqTransferFunctionWidgetPropertyDialog.ui
  Resources/UI/pqVCRToolbar.ui
  Resources/UI/pqViewResolutionPropertyWidget.ui
  Resources/UI/pqWelcomeDialog.ui
  Resources/UI/pqYoungsMaterialPropertyWidget.ui)

set(resource_files
  Resources/pqApplicationComponents.qrc)

set(qt_components
  Network
  Widgets
  Svg)

if (paraview_use_materialeditor)
  list(APPEND classes
    pqMaterialAttributesDelegate
    pqMaterialEditor
    pqNewMaterialDialog)

  list(APPEND ui_files
    Resources/UI/pqMaterialEditor.ui
    Resources/UI/pqNewMaterialDialog.ui)
endif ()

if (PARAVIEW_USE_QTHELP)
  list(APPEND qt_components
    Help)

  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND old_classes
    pqHelpReaction
    pqPluginDocumentationBehavior)
endif ()

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  ${qt_components})

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

vtk_module_add_module(ParaView::pqApplicationComponents
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${ui_files}
          ${resource_files})
vtk_module_link(ParaView::pqApplicationComponents
  PRIVATE
    Qt5::Widgets
    Qt5::Svg
    Qt5::Network)
vtk_module_definitions(ParaView::pqApplicationComponents PRIVATE QT_NO_KEYWORDS)
if (PARAVIEW_USE_QTHELP)
  vtk_module_link(ParaView::pqApplicationComponents
    PRIVATE
      Qt5::Help)
endif ()
