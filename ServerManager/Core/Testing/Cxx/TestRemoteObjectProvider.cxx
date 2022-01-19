/*=========================================================================

  Program:   ParaView
  Module:    TestRemoteObjectProvider.h

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
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyTemplate.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
// #include "vtkSMPropertyHelper.h"

bool DoTest(vtkBuiltinSession* session, rxcpp::schedulers::run_loop& runLoop)
{
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

  auto* pm = vtkProcessModule::GetProcessModule();
  pm->WaitForExit(runLoop, std::chrono::seconds(2));
  return true;
}

int TestRemoteObjectProvider(int argc, char* argv[])
{
  vtkProcessModule::ProcessTypes type = vtkProcessModule::PROCESS_BATCH;
  rxcpp::schedulers::run_loop runLoop;
  vtkProcessModule::Initialize(type, argc, argv, rxcpp::observe_on_run_loop(runLoop));
  auto* pm = vtkProcessModule::GetProcessModule();

  // start the server session. this will start the services.
  auto session = vtk::TakeSmartPointer(vtkBuiltinSession::New());
  if (pm->GetRank() == 0)
  {
    pm->Exit(DoTest(session, runLoop) ? EXIT_SUCCESS : EXIT_FAILURE);
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
