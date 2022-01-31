/*=========================================================================

  Program:   ParaView
  Module:    TestPropertyIterator.cxx

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
#include "vtkPVApplication.h"
#include "vtkPVApplicationOptions.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <cstdlib>
#include <vtk_cli11.h>

bool DoPropertyIteratorTest(vtkClientSession* session, rxcpp::schedulers::run_loop& runLoop)
{
  auto* pxm = session->GetProxyManager();
  auto proxy = pxm->NewProxy("sources", "SphereSource");

  vtkLogF(INFO, "UNORDERED:");
  for (const auto& item : proxy->GetProperties())
  {
    vtkLogF(INFO, " name %s propery %p", item.GetName().c_str(), item.GetProperty().GetPointer());
  }

  vtkLogF(INFO, "ORDERED:");
  for (const auto& item : proxy->GetOrderedProperties())
  {
    vtkLogF(INFO, " name %s propery %p", item.GetName().c_str(), item.GetProperty().GetPointer());
  }

  return true;
}

int TestPropertyIterator(int argc, char* argv[])
{
  // Create the primary event-loop for the application.
  rxcpp::schedulers::run_loop runLoop;

  // initialize the MPI environment.
  vtkDistributedEnvironment environment(&argc, &argv);

  // determine what mode this test is running under
  // and create appropriate application type.
  vtkNew<vtkPVApplication> app;

  // process command line arguments.
  CLI::App cli("Test Property Iterator");

  // let's use nicer formatter for help text.
  vtkPVApplicationOptions::SetupDefaults(cli);

  app->GetOptions()->Populate(cli);

  try
  {
    cli.parse(argc, argv);
  }
  catch (const CLI::ParseError& e)
  {
    app->Exit(app->GetRank() == 0 ? cli.exit(e) : EXIT_SUCCESS);
  }

  app->Initialize(argv[0], rxcpp::observe_on_run_loop(runLoop), environment.GetController());

  auto id = app->CreateBuiltinSession();
  bool success = DoPropertyIteratorTest(app->GetSession(id), runLoop);
  app->Finalize();
  if (!success)
  {
    return EXIT_FAILURE;
  }
  return app->GetExitCode();
}
