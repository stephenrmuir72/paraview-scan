/*=========================================================================

  Program:   ParaView
  Module:    TestDataArraySelectionWrapping.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientSession.h"
#include "vtkDataArraySelection.h"
#include "vtkDistributedEnvironment.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVApplication.h"
#include "vtkPVApplicationOptions.h"
#include "vtkPVDataInformation.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTestUtilities.h"

#include <vtk_cli11.h>

bool DoDataArraySelectionTest(
  vtkClientSession* session, const std::string& filename, rxcpp::schedulers::run_loop& runLoop)
{
  auto* app = vtkPVCoreApplication::GetInstance();
  auto* pxm = session->GetProxyManager();
  auto proxy = vtk::TakeSmartPointer(pxm->NewProxy("sources", "IOSSReader"));
  vtkSMPropertyHelper(proxy, "FileName").Set(filename.c_str());
  proxy->UpdateVTKObjects();

  proxy->UpdateInformation().subscribe(
    [](bool changed) { vtkLogF(INFO, "proxy properties have been updated (%d)", changed); });

  app->WaitForExit(runLoop, std::chrono::seconds(1));

  const std::vector<std::string> expectedValues = { "accl", "1", "displ", "1", "vel", "1" };

  vtkSMPropertyHelper helper(proxy, "NodeBlockFieldInfo");
  std::vector<std::string> values;
  for (int i = 0; i < helper.GetNumberOfElements(); i++)
  {
    values.push_back(helper.GetAsString(i));
  }

  if (values != expectedValues)
  {
    return false;
  }

  return true;
}

int TestDataArraySelectionWrapping(int argc, char* argv[])
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
  CLI::App cli("Test RemoteOject Provider");

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
      bool success = DoDataArraySelectionTest(app->GetSession(id), filename, runLoop);
      app->Exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
    });
  app->WaitForExit(runLoop, std::chrono::seconds(1));
  app->Finalize();
  return app->GetExitCode();
}
