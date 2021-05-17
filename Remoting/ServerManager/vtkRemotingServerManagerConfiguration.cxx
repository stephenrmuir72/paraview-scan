/*=========================================================================

  Program:   ParaView
  Module:    vtkRemotingServerManagerConfiguration.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRemotingServerManagerConfiguration.h"

#include "vtkCLIOptions.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"

#include <vtk_cli11.h>

vtkStandardNewMacro(vtkRemotingServerManagerConfiguration);
//----------------------------------------------------------------------------
vtkRemotingServerManagerConfiguration::vtkRemotingServerManagerConfiguration()
  : MultiClientMode(false)
{
}

//----------------------------------------------------------------------------
vtkRemotingServerManagerConfiguration::~vtkRemotingServerManagerConfiguration() {}

//----------------------------------------------------------------------------
vtkRemotingServerManagerConfiguration* vtkRemotingServerManagerConfiguration::GetInstance()
{
  static vtkSmartPointer<vtkRemotingServerManagerConfiguration> Singleton =
    vtk::TakeSmartPointer(vtkRemotingServerManagerConfiguration::New());
  return Singleton.GetPointer();
}

//----------------------------------------------------------------------------
bool vtkRemotingServerManagerConfiguration::PopulateOptions(
  vtkCLIOptions* options, vtkProcessModule::ProcessTypes ptype)
{
  auto app = options->GetCLI11App();
  auto group =
    app->add_option_group("ParaView ServerManager", "Options affecting ParaView's Server-Manager");
  switch (ptype)
  {
    case vtkProcessModule::PROCESS_SERVER:
    case vtkProcessModule::PROCESS_DATA_SERVER:
      group->add_flag("--multi-clients", this->MultiClientMode,
        "Enable connecting to multiple clients concurrently in a collaborative mode "
        "(experimental).");
      break;
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkRemotingServerManagerConfiguration::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultiClientMode: " << this->MultiClientMode << endl;
}
