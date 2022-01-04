/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumServicesEngine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThalliumServicesEngine.h"

#include "vtkObjectFactory.h"
#include "vtkThalliumServicesEngineInternals.h"

vtkStandardNewMacro(vtkThalliumServicesEngine);
//----------------------------------------------------------------------------
vtkThalliumServicesEngine::vtkThalliumServicesEngine() = default;

//----------------------------------------------------------------------------
vtkThalliumServicesEngine::~vtkThalliumServicesEngine() = default;

//----------------------------------------------------------------------------
std::string vtkThalliumServicesEngine::InitializeInternal(const std::string& url)
{
  this->Internals.reset(new vtkThalliumServicesEngineInternals(url, this->GetController()));
  return this->Internals->GetUrl();
}

//----------------------------------------------------------------------------
void vtkThalliumServicesEngine::FinalizeInternal()
{
  this->Internals.reset();
}

//----------------------------------------------------------------------------
vtkThalliumServicesEngineInternals* vtkThalliumServicesEngine::GetInternals()
{
  return this->Internals.get();
}

//----------------------------------------------------------------------------
void vtkThalliumServicesEngine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
