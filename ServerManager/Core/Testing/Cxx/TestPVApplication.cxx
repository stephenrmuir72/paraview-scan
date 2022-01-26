/*=========================================================================

  Program: ParaView
  Module:  TestPVApplication.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientSession.h"
#include "vtkDistributedEnvironment.h"
#include "vtkPVApplication.h"
#include "vtkPVApplicationOptions.h"
#include "vtkPVServerApplication.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyTemplate.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#include <vtk_cli11.h>

bool DoClientTest(vtkTypeUInt32 sessionId, rxcpp::schedulers::run_loop& runLoop)
{
  auto* app = vtkPVApplication::GetInstance();
  auto* session = app->GetSession(sessionId);
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

  app->WaitForExit(runLoop, std::chrono::seconds(2));
  return true;
}

int TestPVApplication(int argc, char* argv[])
{
  // Create the primary event-loop for the application.
  rxcpp::schedulers::run_loop runLoop;

  // initialize the MPI environment.
  vtkDistributedEnvironment environment(&argc, &argv);

  // determine what mode this test is running under
  // and create appropriate application type.
  std::vector<char*> args;
  vtkSmartPointer<vtkPVCoreApplication> app;
  for (int cc = 0; cc < argc; ++cc)
  {
    if (strcmp(argv[cc], "--server") == 0)
    {
      app = vtk::TakeSmartPointer(vtkPVServerApplication::New());
    }
    else if (strcmp(argv[cc], "--client") == 0)
    {
      app = vtk::TakeSmartPointer(vtkPVApplication::New());

      // a quick way to enqueue tasks on the main loop .
      rxcpp::observable<>::just(1)
        .observe_on(rxcpp::observe_on_run_loop(runLoop))
        .subscribe([&](int) {
          auto* pvapp = vtkPVApplication::GetInstance();
          auto sessionId = pvapp->CreateBuiltinSession();
          DoClientTest(sessionId, runLoop);
        });
    }
    else if (strcmp(argv[cc], "--batch") == 0)
    {
    }
    else
    {
      args.push_back(argv[cc]);
    }
  }

  // process command line arguments.
  CLI::App cli("Test application");

  // let's use nicer formatter for help text.
  vtkPVApplicationOptions::SetupDefaults(cli);

  // populate CLI with ParaView options
  app->GetOptions()->Populate(cli);

  try
  {
    cli.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    app->Exit(app->GetRank() == 0 ? cli.exit(e) : EXIT_SUCCESS);
  }

  app->Initialize(args[0], rxcpp::observe_on_run_loop(runLoop), environment.GetController());
  app->WaitForExit(runLoop, std::chrono::seconds(2));
  app->Finalize();
  return app->GetExitCode();
}
