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
  vtkClientSession
  vtkDisplayConfiguration
  vtkDistributedEnvironment
  vtkInformationProvider
  vtkObjectStore
  vtkObjectWrapper
  vtkProxyDefinitionManager
  vtkPVApplication
  vtkPVApplicationOptions
  vtkPVArrayInformation
  vtkPVCoreApplication
  vtkPVDataAssemblyInformation
  vtkPVDataInformation
  vtkPVDataSetAttributesInformation
  vtkPVInformation
  vtkPVLogInformation
  vtkPVMemoryUseInformation
  vtkPVServerApplication
  vtkPVServerOptions
  vtkPVSystemConfigInformation
  vtkPVTemporalDataInformation
  vtkPVTimerInformation
  vtkReflectionInitializer
  vtkRemoteObjectProvider
  vtkServerSession
  vtkSession
  vtkSMCommand
  vtkSMDomain
  vtkSMDomainIterator
  vtkSMDomainRange
  vtkSMDoubleRangeDomain
  vtkSMDoubleVectorProperty
  vtkSMIdTypeVectorProperty
  vtkSMInputProperty
  vtkSMInstantiator
  vtkSMIntRangeDomain
  vtkSMIntVectorProperty
  vtkSMOrderedPropertyIterator
  vtkSMOrderedPropertyRange
  vtkSMOutputPort
  vtkSMPrototypeProxiesIterator
  vtkSMPrototypeProxiesRange
  vtkSMProperty
  vtkSMPropertyIterator
  vtkSMPropertyRange
  vtkSMProxy
  vtkSMProxyDefinitionsIterator
  vtkSMProxyDefinitionsRange
  vtkSMProxy
  vtkSMProxyManager
  vtkSMProxyProperty
  vtkSMRegisteredProxiesIterator
  vtkSMRegisteredProxiesRange
  vtkSMStringVectorProperty
  vtkStringUtilities
)
set(template_classes
  vtkSMPropertyTemplate
  vtkSMRangeDomainTemplate
)

set(old_classes
  vtkPResourceFileLocator
  vtkPVServerInformation
  vtkRemotingCoreConfiguration
  vtkPVDynamicInitializerPluginInterface
  vtkPVFileInformation
  vtkPVFileInformationHelper
  vtkPVPlugin
  vtkPVPluginLoader
  vtkPVPluginsInformation
  vtkPVPluginTracker
  vtkPVPythonInformation
  vtkPVPythonModule
  vtkPVPythonPluginInterface
  vtkPVServerManagerPluginInterface
)
set(sources)
set(private_headers
  vtkSMPropertyProducer.h)

set(headers
  vtkParaViewDeprecation.h)

if (APPLE)
  list(APPEND sources
    vtkPVMacFileInformationHelper.mm)
  list(APPEND private_headers
    vtkPVMacFileInformationHelper.h)
endif ()

set(nowrap_headers)

set_property(SOURCE vtkPVPluginLoader.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "BUILD_SHARED_LIBS=$<BOOL:${BUILD_SHARED_LIBS}>")

if (WIN32)
  set(libdir_relpath ".")
else ()
  file(RELATIVE_PATH libdir_relpath
    "/prefix/${_vtk_build_RUNTIME_DESTINATION}"
    "/prefix/${_vtk_build_LIBRARY_DESTINATION}")
endif ()
set_property(SOURCE vtkPVPluginTracker.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS
      "BUILD_SHARED_LIBS=$<BOOL:${BUILD_SHARED_LIBS}>"
      "PARAVIEW_RELATIVE_LIBPATH=\"${libdir_relpath}\""
      "PARAVIEW_SUBDIR=\"paraview${paraview_version_suffix}/plugins\"")

set(vtkPVCoreApplication_defines "BUILD_SHARED_LIBS=$<BOOL:${BUILD_SHARED_LIBS}>")
if (PARAVIEW_INITIALIZE_MPI_ON_CLIENT)
  list(APPEND vtkPVCoreApplication_defines
    PARAVIEW_INITIALIZE_MPI_ON_CLIENT)
endif ()
set_source_files_properties(vtkPVCoreApplication.cxx
  PROPERTIES
    COMPILE_DEFINITIONS "${vtkPVCoreApplication_defines}")

# TODO: Expose CMake options for these?
set(vtkPVOptions_defines)
if (PARAVIEW_ALWAYS_SECURE_CONNECTION)
  list(APPEND vtkPVOptions_defines
    PARAVIEW_ALWAYS_SECURE_CONNECTION)
endif ()
if (PARAVIEW_ENABLE_FPE)
  list(APPEND vtkPVOptions_defines
    PARAVIEW_ENABLE_FPE)
endif ()
if (VTK_OPENGL_HAS_EGL)
  list(APPEND vtkPVOptions_defines
    VTK_OPENGL_HAS_EGL)
endif ()
set_source_files_properties(
  vtkPVOptions.cxx
  vtkPVApplicationOptions.cxx
  PROPERTIES
    COMPILE_DEFINITIONS "${vtkPVOptions_defines}")

set(PARAVIEW_PLUGIN_LOADER_PATHS ""
  CACHE STRING "Extra paths to search for plugins")
mark_as_advanced(PARAVIEW_PLUGIN_LOADER_PATHS)
if (PARAVIEW_PLUGIN_LOADER_PATHS)
  set_source_files_properties(vtkPVPluginLoader.cxx
    PROPERTIES
      COMPILE_DEFINITIONS PARAVIEW_PLUGIN_LOADER_PATHS=\"${PARAVIEW_PLUGIN_LOADER_PATHS}\")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVVersion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersion.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersion.h")

vtk_module_add_module(ParaView::ServerManagerCore
  CLASSES ${classes}
  TEMPLATE_CLASSES  ${template_classes}
  SOURCES ${sources}
  HEADERS ${headers}
  NOWRAP_HEADERS ${nowrap_headers}
  PRIVATE_HEADERS ${private_headers})

# we don't want to wrap this module for CS.
vtk_module_client_server_exclude()

# link with compiled proxy definitions
vtk_module_link(ParaView::ServerManagerCore
  PRIVATE
    paraview_server_manager)

paraview_add_server_manager_xmls(
  MODULE VTK::FiltersSources
  XMLS   XML/VTK_FiltersSources.xml)

paraview_add_server_manager_xmls(
  MODULE VTK::FiltersGeneral
  XMLS   XML/VTK_FiltersGeneral.xml)

# for vtkPVFileInformation
if (WIN32)
  vtk_module_link(ParaView::ServerManagerCore
    PRIVATE
      mpr)
elseif (APPLE)
  vtk_module_link(ParaView::ServerManagerCore PUBLIC "-framework Foundation")
endif ()
