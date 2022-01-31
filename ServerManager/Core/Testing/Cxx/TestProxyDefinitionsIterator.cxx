/*=========================================================================

  Program:   ParaView
  Module:    TestProxyDefinitionsIterator.cxx

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
#include "vtkPVXMLElement.h"
#include "vtkProxyDefinitionManager.h"

#include <vtk_cli11.h>

bool DoProxyDefinitionsIteratorTest(vtkClientSession* session)
{
  auto* pxd = session->GetProxyDefinitionManager();

  for (const auto& item : pxd->GetDefinitions())
  {
    vtkLogF(INFO, " group %s name %s definition %p", item.GetGroup().c_str(),
      item.GetName().c_str(), item.GetDefinition().GetPointer());
  }
  std::cout << "---------------------------\n";
  for (const auto& item : pxd->GetDefinitions("internal_filters"))
  {
    vtkLogF(INFO, "NEXT");

    vtkLogF(INFO, " group %s name %s definition %p", item.GetGroup().c_str(),
      item.GetName().c_str(), item.GetDefinition().GetPointer());
  }

  return true;
}

int TestProxyDefinitionsIterator(int argc, char* argv[])
{
  // Create the primary event-loop for the application.
  rxcpp::schedulers::run_loop runLoop;

  // determine what mode this test is running under
  // and create appropriate application type.
  vtkNew<vtkPVApplication> app;

  // process command line arguments.
  CLI::App cli("Test ProxyDefinitions Iterator");

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

  app->Initialize(argv[0], rxcpp::observe_on_run_loop(runLoop));

  auto id = app->CreateBuiltinSession();
  DoProxyDefinitionsIteratorTest(app->GetSession(id));
  app->Finalize();
  return app->GetExitCode();
}
