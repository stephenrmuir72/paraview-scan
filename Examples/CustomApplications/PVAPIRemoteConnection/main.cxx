#include <vtkDummyController.h>
#include <vtkInitializationHelper.h>
#include <vtkLogger.h>
#include <vtkMultiProcessController.h>
#include <vtkPVView.h>
#include <vtkProcessModule.h>
#include <vtkRemotingCoreConfiguration.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMSession.h>
#include <vtkSMSessionClient.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSmartPointer.h>
#include <vtkStringFormatter.h>
#include <vtkStringScanner.h>

#include <string>

namespace
{

vtkSmartPointer<vtkSMSessionClient> Initialize(const std::string& serverName, int port)
{
  vtkInitializationHelper::SetApplicationName("SandboxClient");
  vtkInitializationHelper::Initialize("SandboxClient", vtkProcessModule::PROCESS_CLIENT);

  // Trigger connection request to give url
  vtkSmartPointer<vtkSMSessionClient> session = vtkSmartPointer<vtkSMSessionClient>::New();
  std::string serverURL = vtk::format("cs://{}:{}", serverName, port);
  if (!session->Connect(serverURL.c_str()))
  {
    vtkLogF(ERROR, "Could not connect to server with URL: %s", serverURL.c_str());
    return nullptr;
  }

  // Setup the objects that needs the session handle
  vtkProcessModule::GetProcessModule()->RegisterSession(session);
  vtkSMProxyManager::GetProxyManager()->SetActiveSession(session);

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeSession(session);

  return session;
}

void Shutdown(vtkSmartPointer<vtkSMSessionClient> session)
{
  // Call disconnection event and unregister the session automatically.
  vtkSMSessionClient::Disconnect(session);

  vtkInitializationHelper::Finalize();
}

void MainLoop(vtkSmartPointer<vtkSMSessionClient> session,
  vtkSmartPointer<vtkSMParaViewPipelineControllerWithRendering> controller)
{
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  // Create a render view
  vtkSmartPointer<vtkSMRenderViewProxy> renderView = vtkSmartPointer<vtkSMRenderViewProxy>::Take(
    vtkSMRenderViewProxy::SafeDownCast(pxm->NewProxy("views", "RenderView")));
  controller->InitializeProxy(renderView);
  controller->RegisterViewProxy(renderView);
  renderView->MakeRenderWindowInteractor();
  renderView->UpdateVTKObjects();

  // Create a source for the render view
  vtkSmartPointer<vtkSMSourceProxy> coneSource = vtkSmartPointer<vtkSMSourceProxy>::Take(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "ConeSource")));
  controller->RegisterPipelineProxy(coneSource);

  controller->Show(coneSource, 0, renderView);

  // Start event loop
  renderView->GetInteractor()->Start();
}

}

int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    vtkLogF(ERROR, "Usage ./RemoteConnection serverName serverPort");
    return EXIT_FAILURE;
  }

  std::string serverName = argv[1];
  std::string serverPortStr = argv[2];
  int serverPort;
  vtk::from_chars(serverPortStr.data(), serverPortStr.data() + serverPortStr.size(), serverPort);

  // Initializing connection
  vtkSmartPointer<vtkSMSessionClient> session = Initialize(serverName, serverPort);
  if (!session)
  {
    vtkLogF(ERROR, "Connection failed, aborting.");
    return EXIT_FAILURE;
  }

  // Setup dummy controller
  vtkNew<vtkDummyController> multiProcessController;
  vtkMultiProcessController::SetGlobalController(multiProcessController);

  // Starting event loop
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  MainLoop(session, controller);

  // Clear all resources
  Shutdown(session);

  return EXIT_SUCCESS;
}
