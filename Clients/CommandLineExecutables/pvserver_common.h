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
#include "vtkPVConfig.h"

#include "vtkCLIOptions.h"
#include "vtkInitializationHelper.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVSessionServer.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#ifdef PARAVIEW_USE_PYTHON
extern "C" {
void vtkPVInitializePythonModules();
}
#endif

#include "ParaView_paraview_plugins.h"

static bool RealMain(int argc, char* argv[], vtkProcessModule::ProcessTypes type)
{
  auto cliApp = vtk::TakeSmartPointer(vtkCLIOptions::New());
  switch (type)
  {
    case vtkProcessModule::PROCESS_DATA_SERVER:
      cliApp->SetDescription("ParaView Data-Server");
      break;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
      cliApp->SetDescription("ParaView Render-Server");
      break;

    case vtkProcessModule::PROCESS_SERVER:
      cliApp->SetDescription(R"--(
ParaView Server
===============

This is the ParaView Server executable. This is intended for client-server use-cases
which require the client and server to be on different processes, potentially on different
systems.

Typically, one connects a ParaView client (either a graphical client, or a Python-based client)
to this process to drive the data analysis and visualization pipelines.
)--");
      break;
  }

  // Init current process type
  auto status = vtkInitializationHelper::Initialize(argc, argv, type, cliApp);
  switch (status)
  {
    case vtkInitializationHelper::Fail:
      // TODO: print error
      break;

    case vtkInitializationHelper::HelpRequested:
      cout << cliApp->GetHelp().c_str() << endl;
      break;

    case vtkInitializationHelper::VersionRequested:
      cout << vtkSMProxyManager::GetParaViewSourceVersion() << endl;
      break;
  }
  cliApp = nullptr;
  if (status != vtkInitializationHelper::Success)
  {
    vtkInitializationHelper::Finalize();
    return status != vtkInitializationHelper::Fail;
  }

  auto config = vtkRemotingCoreConfiguration::GetInstance();

#ifdef PARAVIEW_USE_PYTHON
  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();
#endif

  // register static plugins
  ParaView_paraview_plugins_initialize();

  vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLs("paraview");

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkMultiProcessController* controller = pm->GetGlobalController();

  vtkPVSessionServer* session = vtkPVSessionServer::New();
  session->SetMultipleConnection(config->GetMultiClientMode());
  session->SetDisableFurtherConnections(config->GetDisableFurtherConnections());

  int process_id = controller->GetLocalProcessId();
  if (process_id == 0)
  {
    // Report status:
    if (config->GetReverseConnection())
    {
      cout << "Connecting to client (reverse connection requested)..." << endl;
    }
    else
    {
      cout << "Waiting for client..." << endl;
    }
  }
  bool success = false;
  if (session->Connect())
  {
    success = true;
    pm->RegisterSession(session);
    if (controller->GetLocalProcessId() == 0)
    {
      while (pm->GetNetworkAccessManager()->ProcessEvents(0) != -1)
      {
      }
    }
    else
    {
      controller->ProcessRMIs();
    }
    pm->UnRegisterSession(session);
  }

  cout << "Exiting..." << endl;
  session->Delete();
  // Exit application
  vtkInitializationHelper::Finalize();
  return success;
}
