/*=========================================================================

  Program:   ParaView
  Module:    TestProxyManagerIterator.cxx

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
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <vtk_cli11.h>

#define VALIDATE(x)                                                                                \
  if ((x))                                                                                         \
  {                                                                                                \
    vtkLogF(INFO, "Validating : " #x "...success!");                                               \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    vtkLogF(INFO, "Validating : " #x "...failed!");                                                \
    return false;                                                                                  \
  }

bool DoProxyManagerIteratorTest(vtkClientSession* session)
{
  auto* pxm = session->GetProxyManager();
  auto proxy = pxm->NewProxy("sources", "SphereSource");
  pxm->RegisterProxy("sources", "SphereSource", proxy);

  proxy = pxm->NewProxy("sources", "SphereSource");
  pxm->RegisterProxy("sources", "SphereSource", proxy);

  proxy = pxm->NewProxy("sources", "PointSource");
  pxm->RegisterProxy("sources", "PointSource", proxy);

  proxy = pxm->NewProxy("filters", "CellDerivatives");
  pxm->RegisterProxy("filters", "CellDerivatives", proxy);

  proxy = pxm->NewProxy("filters", "CellCenters");
  pxm->RegisterProxy("filters", "CellCenters", proxy);

  {
    const int expectedItems = 5;
    int items = 0;
    vtkLogF(INFO, "Registered");
    for (const auto& item : pxm->GetRegisteredProxies())
    {
      vtkLogF(INFO, " name %s group %s proxy %p", item.GetGroup().c_str(), item.GetName().c_str(),
        item.GetProxy().GetPointer());
      items++;
    }
    VALIDATE(items == expectedItems)
  }

  {
    const int expectedItems = 3;
    int items = 0;
    vtkLogF(INFO, "Registered within specific group");
    for (const auto& item : pxm->GetRegisteredProxies("sources"))
    {
      vtkLogF(INFO, " name %s group %s proxy %p", item.GetGroup().c_str(), item.GetName().c_str(),
        item.GetProxy().GetPointer());
      items++;
      VALIDATE(item.GetGroup() == "sources");
    }
    VALIDATE(items == expectedItems)
  }

  // FIXME update once we gave prototype proxies
  for (const auto& item : pxm->GetPrototypeProxies())
  {
    vtkLogF(INFO, " name %s group %s proxy %p", item.GetGroup().c_str(), item.GetName().c_str(),
      item.GetProxy().GetPointer());
  }
  return true;
}

int TestProxyManagerIterator(int argc, char* argv[])
{
  // Create the primary event-loop for the application.
  rxcpp::schedulers::run_loop runLoop;

  // initialize the MPI environment.
  vtkDistributedEnvironment environment(&argc, &argv);

  // determine what mode this test is running under
  // and create appropriate application type.
  vtkNew<vtkPVApplication> app;

  // process command line arguments.
  CLI::App cli("Test ProxyManager Iterator");

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
  bool success = DoProxyManagerIteratorTest(app->GetSession(id));
  app->Finalize();
  if (!success)
  {
    return EXIT_FAILURE;
  }
  return app->GetExitCode();
}
