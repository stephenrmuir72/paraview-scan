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
  vtkClientSessionViews
  vtkGeometryRepresentation
  vtkOrderedCompositingHelper
  vtkPVAxesActor
  vtkPVAxesWidget
  vtkPVCenterAxesActor
  vtkPVDataDeliveryManager
  vtkPVDataRepresentation
  vtkPVDataRepresentationPipeline
  vtkPVDefaultPass
  vtkPVDiscretizableColorTransferFunction
  vtkPVGeometryFilter
  vtkPVLight
  vtkPVLODActor
  vtkPVMaterialLibrary
  vtkPVOpenGLInformation
  vtkPVProcessWindow
  vtkPVRecoverGeometryWireframe
  vtkPVRenderingCapabilitiesInformation
  vtkPVRenderView
  vtkPVRenderViewSettings
  vtkPVSynchronizedRenderer
  vtkPVView
  vtkRemoteObjectProviderViews
  vtkSMApplyController
  vtkSMMaterialDomain
  vtkSMParaViewPipelineControllerWithRendering
  vtkSMRendererDomain
  vtkSMRenderViewProxy
  vtkSMTransferFunctionManager
  vtkSMTransferFunctionPresets
  vtkSMTransferFunctionProxy
  vtkSMUtilities
  vtkSMViewLayoutProxy
  vtkSMViewProxy
  vtkTilesHelper
  vtkViewLayout
)

set(private_headers)
set(sources)

if (TARGET ParaView::icet)
  list(APPEND classes
    vtkIceTCompositePass
    vtkIceTContext
    vtkIceTSynchronizedRenderers)

  # Encode glsl files.
  foreach (file vtkIceTCompositeZPassShader_fs.glsl)
    get_filename_component(file_we ${file} NAME_WE)
    vtk_encode_string(
      INPUT         "${file}"
      NAME          "${file_we}"
      EXPORT_SYMBOL "VTKREMOTINGSERVERMANAGERVIEWS_EXPORT"
      EXPORT_HEADER "vtkRemotingServerManagerViewsModule.h"
      HEADER_OUTPUT header
      SOURCE_OUTPUT source)
    list(APPEND sources ${source})
    list(APPEND private_headers ${header})
  endforeach ()
endif()

# Compile ColorMaps.json
vtk_encode_string(
  INPUT     "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json"
  NAME      "vtkSMTransferFunctionPresetsBuiltin"
  EXPORT_SYMBOL "VTKREMOTINGSERVERMANAGERVIEWS_EXPORT"
  EXPORT_HEADER "vtkRemotingServerManagerViewsModule.h"
  HEADER_OUTPUT colormaps_header
  SOURCE_OUTPUT colormaps_source)
list(APPEND sources "${colormaps_source}")
list(APPEND private_headers "${colormaps_header}")

vtk_object_factory_declare(
  BASE "vtkClientSession"
  OVERRIDE "vtkClientSessionViews")

vtk_object_factory_declare(
  BASE "vtkRemoteObjectProvider"
  OVERRIDE "vtkRemoteObjectProviderViews")

vtk_object_factory_declare(
  BASE "vtkSMParaViewPipelineController"
  OVERRIDE "vtkSMParaViewPipelineControllerWithRendering")

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKREMOTINGSERVERMANAGERVIEWS_EXPORT")

vtk_module_add_module(ParaView::RemotingServerManagerViews
  CLASSES         ${classes}
  SOURCES         ${sources} ${vtk_object_factory_source}
  PRIVATE_HEADERS ${private_headers} ${vtk_object_factory_header}
)

paraview_add_server_manager_xmls(
  XMLS
    XML/ParaView_ServerManagerViews.xml
    XML/ParaView_ServerManagerViews_settings.xml)
