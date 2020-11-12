/*=========================================================================

  Program:   ParaView
  Module:    vtkInSituPipelinePython.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "vtkPython.h" // must be first
#endif

#include "vtkInSituPipelinePython.h"

#include "vtkCPPythonScriptV2Helper.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"

vtkStandardNewMacro(vtkInSituPipelinePython);
//----------------------------------------------------------------------------
vtkInSituPipelinePython::vtkInSituPipelinePython()
  : FileName(nullptr)
{
}

//----------------------------------------------------------------------------
vtkInSituPipelinePython::~vtkInSituPipelinePython()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Initialize()
{
#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  // call some Python API.
  auto status = Py_IsInitialized();
  (void)status;
#endif
  return this->Helper->PrepareFromScript(this->FileName) && this->Helper->Import() &&
    this->Helper->CatalystInitialize();
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Execute(int timestep, double time)
{
  return this->Helper->CatalystExecute(timestep, time);
}

//----------------------------------------------------------------------------
bool vtkInSituPipelinePython::Finalize()
{
  return this->Helper->CatalystFinalize();
}

//----------------------------------------------------------------------------
void vtkInSituPipelinePython::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
}
