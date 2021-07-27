/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonAPI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPythonAPI.h"
#include "vtkObjectFactory.h"

#include "vtkPVConfig.h" // Required to get build options for paraview

#include "vtkInitializationHelper.h"
#include "vtkPVPluginTracker.h"

#include "ParaView_paraview_plugins.h"

//----------------------------------------------------------------------------
vtkPVPythonAPI::vtkPVPythonAPI() = default;

//----------------------------------------------------------------------------
vtkPVPythonAPI::~vtkPVPythonAPI() = default;

//----------------------------------------------------------------------------
void vtkPVPythonAPI::InitializeParaViewPython()
{
  // Setup options
  vtkInitializationHelper::SetApplicationName("ParaView");

  // register static plugins
  ParaView_paraview_plugins_initialize();

  vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLs("paraview");
}

//----------------------------------------------------------------------------
void vtkPVPythonAPI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
