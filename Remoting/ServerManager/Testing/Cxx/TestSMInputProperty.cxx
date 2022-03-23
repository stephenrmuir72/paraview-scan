/*=========================================================================

  Program:   ParaView
  Module:    TestSMInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCallbackCommand.h"
#include "vtkClientSession.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDistributedEnvironment.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVApplication.h"
#include "vtkPVApplicationOptions.h"
#include "vtkPVDataInformation.h"
#include "vtkSMDomain.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTestUtilities.h"

#include <vtk_cli11.h>

int eventCounter = 0;

void func(vtkObject*, unsigned long eid, void* clientdata, void* /*calldata*/)
{
  eventCounter++;
  vtkSMDomain* domain = static_cast<vtkSMDomain*>(clientdata);
  vtkLogF(INFO, "`%s`  event fired from domain `%s` of Property `%s`",
    vtkCommand::GetStringFromEventId(eid), domain->GetXMLName(),
    domain->GetProperty()->GetXMLName());
}

int DoSMInputPropertyTest(
  vtkClientSession* session, const std::string& filename, rxcpp::schedulers::run_loop& runLoop)
{
  auto* app = vtkPVCoreApplication::GetInstance();
  auto* pxm = session->GetProxyManager();

  auto proxy =
    vtk::TakeSmartPointer(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "IOSSReader")));
  vtkSMPropertyHelper(proxy, "FileName").Set(filename.c_str());
  proxy->UpdateVTKObjects();

  // Disable array initially
  vtkSMPropertyHelper inputArrayStatus(proxy, "ElementBlockFieldInfo");
  inputArrayStatus.Set(0, "eqps");
  inputArrayStatus.Set(0, "0");

  proxy->UpdateInformation().subscribe(
    [](bool changed) { vtkLogF(INFO, "proxy properties have been updated (%d)", changed); });

  auto filter =
    vtk::TakeSmartPointer(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "Threshold")));
  vtkSMPropertyHelper(filter, "Input").Set(proxy, 0);
  vtkSMPropertyHelper(filter, "LowerThreshold").Set(0);
  filter->UpdateVTKObjects();
  filter->UpdatePipeline(0.0);

  // set up a callback that is triggered when the domain is modified
  vtkNew<vtkCallbackCommand> command;
  command->SetCallback(func);
  auto* domainToObserve = filter->GetProperty("SelectInputScalars")->GetDomain("array_list");
  command->SetClientData((void*)domainToObserve);
  domainToObserve->AddObserver(vtkCommand::DomainModifiedEvent, command);

  // enable the  array
  inputArrayStatus.Set(1, "1");
  proxy->UpdateInformation().subscribe(
    [](bool changed) { vtkLogF(INFO, "proxy properties have been updated (%d)", changed); });

  app->WaitForExit(runLoop, std::chrono::seconds(1));

  if (eventCounter == 1)
  {
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int TestSMInputProperty(int argc, char* argv[])
{
  // Create the primary event-loop for the application.
  rxcpp::schedulers::run_loop runLoop;

  // initialize the MPI environment.
  vtkDistributedEnvironment environment(&argc, &argv);
  const int rank = environment.GetRank();

  // determine what mode this test is running under
  // and create appropriate application type.
  vtkNew<vtkPVApplication> app;

  // process command line arguments.
  CLI::App cli("Test SMInputProperty");

  // let's use nicer formatter for help text.
  vtkPVApplicationOptions::SetupDefaults(cli);

  app->GetOptions()->Populate(cli);

  try
  {
    cli.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    return (rank == 0 ? cli.exit(e) : EXIT_SUCCESS);
  }

  app->Initialize(argv[0], rxcpp::observe_on_run_loop(runLoop), environment.GetController());
  char* name = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/can.ex2");
  std::string filename(name);
  delete[] name;

  app->CreateBuiltinSession()
    .observe_on(rxcpp::observe_on_run_loop(runLoop))
    .subscribe([&](vtkTypeUInt32 id) {
      app->Exit(DoSMInputPropertyTest(app->GetSession(id), filename, runLoop));
    });
  app->WaitForExit(runLoop, std::chrono::seconds(1));
  app->Finalize();
  return app->GetExitCode();
}
