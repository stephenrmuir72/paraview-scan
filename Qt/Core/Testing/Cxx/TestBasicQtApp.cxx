/*=========================================================================

  Program: ParaView
  Module:  TestBasicQtApp.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QVTKRenderWindowAdapter.h"
#include "TestBasicQtAppMainWindow.h"
#include "vtkDistributedEnvironment.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPVGUIApplication.h"
#include "vtkPVGUIOptions.h"

#include <QApplication>
#include <QMainWindow>
#include <QSurfaceFormat>
#include <vtk_cli11.h>

int TestBasicQtApp(int argc, char* argv[])
{
  // initialize the MPI environment.
  vtkDistributedEnvironment environment(&argc, &argv);

  if (environment.GetController()->GetNumberOfProcesses() > 1)
  {
    vtkLogF(ERROR, "Qt applications do not supported distributed execution!");
    return EXIT_FAILURE;
  }

  // Set default surface format to ensure Qt creates OpenGL context with correct
  // capabilities. This has to be done before QApplication is created.
  QSurfaceFormat::setDefaultFormat(
    QVTKRenderWindowAdapter::defaultFormat(/*supports_stereo=*/false));

  // Create Qt app.
  // This has to happen after MPI environment is setup but before we process
  // the command line arguments. That ensures that any Qt specific command line
  // arguments are processed and do not affect the ParaView application.
  QApplication qtApp(argc, argv);

  // process command line arguments.
  CLI::App cli("Test application");

  // let's use nicer formatter for help text.
  vtkPVApplicationOptions::SetupDefaults(cli);

  vtkNew<vtkPVGUIApplication> pvApp;

  std::string url;
  cli.add_option("-u,--url", url, "Server url to connect to, if any.");

  pvApp->GetOptions()->Populate(cli);

  // Qt app can directly use this macro since we never execute in distributed
  // mode. App can just exit is CLI parsing fails.
  CLI11_PARSE(cli, argc, argv);

  // Initialize ParaView application
  pvApp->InitializeUsingQt(environment.GetController());

  // Create MainWindow.
  TestBasicQtAppMainWindow window;
  window.connect(url.c_str());
  window.show();

  pvApp->Exit(qtApp.exec());
  pvApp->Finalize();
  return pvApp->GetExitCode();
}
