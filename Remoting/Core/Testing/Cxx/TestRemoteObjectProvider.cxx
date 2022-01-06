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
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

int TestRemoteObjectProvider(int argc, char* argv[])
{
  vtkProcessModule::ProcessTypes type = vtkProcessModule::PROCESS_CLIENT;
  rxcpp::schedulers::run_loop runLoop;
  vtkProcessModule::Initialize(type, argc, argv, rxcpp::observe_on_run_loop(runLoop));
  auto* pm = vtkProcessModule::GetProcessModule();

  // start the server session. this will start the services.
  vtkNew<vtkBuiltinSession> session;

  auto* pxm = session->GetProxyManager();
  auto proxy = pxm->NewProxy("sources", "SphereSource");
  proxy->UpdateVTKObjects();
  proxy->UpdateInformation().subscribe(
    [](bool changed) { vtkLogF(INFO, "proxy properties have been updated (%d)", changed); });

  pm->WaitForExit(runLoop, std::chrono::seconds(5));
  proxy = nullptr;
  pm = nullptr;
  vtkProcessModule::Finalize();
  return EXIT_SUCCESS;
}
