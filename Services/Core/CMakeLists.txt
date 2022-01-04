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

set(headers
  vtkEventual.h
  vtkJson.h
  vtkJsonFwd.h)

set(templates
  vtkServicesEngine.txx)

vtk_module_add_module(ParaView::ServicesCore
  CLASSES     ${classes}
  HEADERS     ${headers}
  TEMPLATES   ${templates})
