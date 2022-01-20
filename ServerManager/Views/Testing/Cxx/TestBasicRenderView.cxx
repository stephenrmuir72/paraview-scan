/*=========================================================================

  Program:   ParaView
  Module:    TestBasicRenderView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBuiltinSession.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyTemplate.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMViewProxy.h"

bool DoTest(vtkBuiltinSession* session, rxcpp::schedulers::run_loop& runLoop)
{
  auto* pm = vtkProcessModule::GetProcessModule();
  auto* pxm = session->GetProxyManager();
  auto proxy = pxm->NewProxy("sources", "SphereSource");
  vtkSMPropertyTemplate<int>::SafeDownCast(proxy->GetProperty("PhiResolution"))
    ->SetElements({ 800 });
  vtkSMPropertyTemplate<int>::SafeDownCast(proxy->GetProperty("ThetaResolution"))
    ->SetElements({ 800 });
  // vtkSMPropertyHelper::SetElements<int>(proxy, "PhiResolution", 800);
  // vtkSMPropertyHelper::SetElements<int>(proxy, "ThetaResolution", 800);
  proxy->UpdateVTKObjects();

  auto shrink = pxm->NewProxy("filters", "ShrinkFilter");
  vtkSMPropertyTemplate<vtkSmartPointer<vtkSMOutputPort>>::SafeDownCast(
    shrink->GetProperty("Input"))
    ->SetElements({ proxy->GetOutputPort(0) });
  shrink->UpdateVTKObjects();

  proxy->UpdateInformation().subscribe(
    [](bool changed) { vtkLogF(INFO, "proxy properties have been updated (%d)", changed); });
  shrink->UpdatePipeline(0.0);

  auto repr = pxm->NewProxy("representations", "GeometryRepresentation");
  vtkSMInputProperty::SafeDownCast(repr->GetProperty("Input"))
    ->SetElements({ shrink->GetOutputPort(0) });
  repr->UpdateVTKObjects();

  auto viewProxy = pxm->NewProxy("views", "RenderView");
  vtkSMProxyProperty::SafeDownCast(viewProxy->GetProperty("Representations"))
    ->AppendElements({ repr });
  viewProxy->UpdateVTKObjects();
  viewProxy->InvokeCommand("Update");

  auto* view = vtkSMViewProxy::SafeDownCast(viewProxy);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(view->GetRenderWindow());
  iren->Initialize();
  while (!iren->GetDone())
  {
    if (pm->WaitForExit(runLoop, std::chrono::milliseconds(5)))
    {
      break;
    }
    iren->ProcessEvents();
  }
  viewProxy = nullptr;
  view = nullptr;
  return true;
}

int TestBasicRenderView(int argc, char* argv[])
{
  vtkProcessModule::ProcessTypes type = vtkProcessModule::PROCESS_BATCH;
  rxcpp::schedulers::run_loop runLoop;
  vtkProcessModule::Initialize(type, argc, argv, rxcpp::observe_on_run_loop(runLoop));
  auto* pm = vtkProcessModule::GetProcessModule();

  // start the server session. this will start the services.
  auto session = vtk::TakeSmartPointer(vtkBuiltinSession::New());
  if (pm->GetRank() == 0)
  {
    auto status = DoTest(session, runLoop);
    pm->WaitForExit(runLoop, std::chrono::milliseconds(100));
    pm->Exit(status ? EXIT_SUCCESS : EXIT_FAILURE);
  }
  else
  {
    pm->WaitForExit(runLoop);
  }

  session = nullptr;
  pm = nullptr;
  vtkProcessModule::Finalize();
  return EXIT_SUCCESS;
}
