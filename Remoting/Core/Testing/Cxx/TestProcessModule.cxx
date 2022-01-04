/*=========================================================================

  Program:   ParaView
  Module:    TestProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkProcessModule.h"
#include "vtkServerSession.h"

int TestProcessModule(int argc, char* argv[])
{
  vtkProcessModule::ProcessTypes type = vtkProcessModule::PROCESS_CLIENT;
  bool test_server = false;
  for (int cc = 0; cc < argc; ++cc)
  {
    if (strcmp(argv[cc], "--server") == 0)
    {
      type = vtkProcessModule::PROCESS_SERVER;
    }
    if (strcmp(argv[cc], "--client") == 0)
    {
      type = vtkProcessModule::PROCESS_CLIENT;
    }
    if (strcmp(argv[cc], "--batch") == 0)
    {
      type = vtkProcessModule::PROCESS_BATCH;
    }
  }

  rxcpp::schedulers::run_loop runLoop;
  vtkProcessModule::Initialize(type, argc, argv, rxcpp::observe_on_run_loop(runLoop));
  auto* pm = vtkProcessModule::GetProcessModule();
  vtkLogIfF(INFO, pm->GetRank() == 0, "Process module initialized.");
  vtkLogIfF(INFO, pm->GetRank() == 0, "   num-ranks: %d", pm->GetNumberOfRanks());
  if (type == vtkProcessModule::PROCESS_SERVER)
  {
    // Setup server session. This will start the services necessary for
    // ParaView server.
    vtkNew<vtkServerSession> session;
    pm->WaitForExit(runLoop, std::chrono::seconds(5));
  }
  pm = nullptr;
  vtkProcessModule::Finalize();
  return EXIT_SUCCESS;
}
