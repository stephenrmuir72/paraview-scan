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
#include "vtkClientSession.h"
#include "vtkDistributedEnvironment.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVApplication.h"
#include "vtkPVApplicationOptions.h"
#include "vtkPVDataInformation.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyTemplate.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <vtk_cli11.h>

bool DoTest(vtkClientSession* session, rxcpp::schedulers::run_loop& runLoop)
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

  auto* app = vtkPVCoreApplication::GetInstance();
  while (!iren->GetDone())
  {
    if (app->WaitForExit(runLoop, std::chrono::milliseconds(5)))
    {
      break;
    }
    iren->ProcessEvents();
  }
  viewProxy = nullptr;
  view = nullptr;
  app->Exit(EXIT_SUCCESS);
  return true;
}

int TestBasicRenderView(int argc, char* argv[])
{
  // Create the primary event-loop for the application.
  rxcpp::schedulers::run_loop runLoop;

  // initialize the MPI environment.
  vtkDistributedEnvironment environment(&argc, &argv);

  // determine what mode this test is running under
  // and create appropriate application type.
  vtkNew<vtkPVApplication> app;

  // process command line arguments.
  CLI::App cli("Test View/Representation application");

  // let's use nicer formatter for help text.
  vtkPVApplicationOptions::SetupDefaults(cli);

  app->GetOptions()->Populate(cli);

  try
  {
    cli.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return (app->GetRank() == 0 ? cli.exit(e) : EXIT_SUCCESS);
  }

  app->Initialize(argv[0], rxcpp::observe_on_run_loop(runLoop), environment.GetController());

  auto id = app->CreateBuiltinSession();
  DoTest(app->GetSession(id), runLoop);
  app->Finalize();
  return app->GetExitCode();
}
