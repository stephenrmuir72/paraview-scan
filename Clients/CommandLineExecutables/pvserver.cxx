/*=========================================================================

Program:   ParaView
Module:    pvserver.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistributedEnvironment.h"
#include "vtkNew.h"
#include "vtkPVServerApplication.h"
#include "vtkPVServerOptions.h"

#include <vtk_cli11.h>

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Create the primary event-loop for the application.
  rxcpp::schedulers::run_loop runLoop;

  // initialize the MPI environment.
  vtkDistributedEnvironment environment(&argc, &argv);

  CLI::App cli(
    "pvserver: the ParaView server\n"
    "=============================\n"
    "This is the ParaView server executable. This is intended for client-server use-cases "
    "which require the client and server to be on different processes, potentially on "
    "different systems.\n\n"
    "Typically, one connects a ParaView client (either a graphical client, or a Python-based "
    "client) to this process to drive the data analysis and visualization pipelines.",
    "pvserver");

  // let's use nicer formatter for help text.
  vtkPVApplicationOptions::SetupDefaults(cli);

  vtkNew<vtkPVServerApplication> app;

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

  app->Initialize(argv[0], rxcpp::observe_on_run_loop(runLoop), environment.GetController());
  app->WaitForExit(runLoop, std::chrono::minutes(3));
  app->Finalize();
  return app->GetExitCode();
}
