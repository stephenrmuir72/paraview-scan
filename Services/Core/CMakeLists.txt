set(classes
  vtkEventualImplementation
  vtkPacket
  vtkServicesEngine
  vtkService
  vtkServiceEndpoint
  vtkRemotingCoreUtilities
  vtkProvider
  vtkChannelSubscription
)

set(nowrap_classes
  vtkReactiveCommand)

set(headers
  vtkEventual.h
  vtkJson.h
  vtkJsonFwd.h)

set(templates
  vtkServicesEngine.txx)

vtk_module_add_module(ParaView::ServicesCore
  CLASSES         ${classes}
  NOWRAP_CLASSES  ${nowrap_classes}
  HEADERS         ${headers}
  TEMPLATES       ${templates})
