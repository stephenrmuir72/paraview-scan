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
  vtkSMAMRLevelsDomain
  vtkSMArrayListDomain
  vtkSMArrayRangeDomain
  vtkSMArraySelectionDomain
  vtkSMBooleanDomain
  vtkSMBoundsDomain
  vtkSMCompositeTreeDomain
  vtkSMCoreUtilities
  vtkSMDataAssemblyDomain
  vtkSMDataAssemblyListDomain
  vtkSMDataTypeDomain
  vtkSMDeserializer
  vtkSMDeserializerXML
  vtkSMDeserializerXMLCache
  vtkSMDimensionsDomain
  vtkSMDiscreteDoubleDomain
  vtkSMDocumentation
  vtkSMDomain
  vtkSMDomainIterator
  vtkSMDoubleRangeDomain
  vtkSMDoubleVectorProperty
  vtkSMEnumerationDomain
  vtkSMExtentDomain
  vtkSMFieldDataDomain
  vtkSMFileListDomain
  vtkSMIdTypeVectorProperty
  vtkSMIndexSelectionDomain
  vtkSMInputArrayDomain
  vtkSMInputFileNameDomain
  vtkSMInputProperty
  vtkSMIntRangeDomain
  vtkSMIntVectorProperty
  vtkSMLink
  vtkSMNamedPropertyIterator
  vtkSMNullProxy
  vtkSMNumberOfComponentsDomain
  vtkSMOrderedPropertyIterator
  vtkSMOutputPort
  vtkSMParaViewPipelineController
  vtkSMProperty
  vtkSMPropertyGroup
  vtkSMPropertyHelper
  vtkSMPropertyIterator
  vtkSMPropertyLink
  vtkSMProxy
  vtkSMProxyDefinitionsIterator
  vtkSMProxyDefinitionsRange
  vtkSMProxyGroupDomain
  vtkSMProxyInitializationHelper
  vtkSMProxyIterator
  vtkSMProxyLink
  vtkSMProxyListDomain
  vtkSMProxyLocator
  vtkSMProxyProperty
  vtkSMProxySelectionModel
  vtkSMRegistrationNamesDomain
  vtkSMSelectionQueryDomain
  vtkSMSessionProxyManager
  vtkSMSourceProxy
  vtkSMStateLoader
  vtkSMStringListDomain
  vtkSMStringVectorProperty
  vtkSMTimeStepIndexDomain
  vtkSMTrace
  vtkSMUncheckedPropertyHelper
  vtkSMVectorProperty
  vtkStringUtilities
)

set(old_classes
  vtkLoadStateOptions
  vtkRemoteWriterHelper
  vtkPVCatalystChannelInformation
  vtkPVCatalystSessionCore
  vtkPVDataMover
  vtkPVFilePathEncodingHelper
  vtkSMCompoundProxyDefinitionLoader
  vtkSMCompoundSourceProxy
  vtkSMCompoundSourceProxyDefinitionBuilder
  vtkSMDataExtractWriterProxy
  vtkSMDataSourceProxy
  vtkSMDeserializerProtobuf
  vtkSMDirectoryProxy
  vtkSMExtractsController
  vtkSMExtractTriggerProxy
  vtkSMExtractWriterProxy
  vtkSMFileUtilities
  vtkSMLoadStateOptionsProxy
  vtkSMMultiplexerSourceProxy
  vtkSMMultiplexerInputDomain
  vtkSMPluginLoaderProxy
  vtkSMPluginManager
  vtkSMPropertyModificationUndoElement
  vtkSMProxyManagerUtilities
  vtkSMPSWriterProxy
  vtkSMProxyManager
  vtkSMPWriterProxy
  vtkSMReaderFactory
  vtkSMReaderReloadHelper
  vtkSMRemoteObjectUpdateUndoElement
  vtkSMSelfGeneratingSourceProxy
  vtkSMSettings
  vtkSMSettingsProxy
  vtkSMStateVersionController
  vtkSMTimeKeeper
  vtkSMTimeKeeperProxy
  vtkSMUndoElement
  vtkSMUndoStack
  vtkSMUndoStackBuilder
  vtkSMWriterFactory
  vtkSMWriterProxy)

set(headers
  vtkParaViewDeprecation.h
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVVersion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersion.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPVVersion.h")

set(private_headers
  vtkSMPropertyInternals.h
  vtkSMProxyInternals.h
  vtkSMProxyPropertyInternals.h
  vtkSMSessionProxyManagerInternals.h)


set(template_classes
  vtkSMRangeDomainTemplate)

vtk_module_add_module(ParaView::RemotingServerManager
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  TEMPLATE_CLASSES ${template_classes})

# link with compiled proxy definitions
vtk_module_link(ParaView::RemotingServerManager
  PRIVATE
    paraview_server_manager)
