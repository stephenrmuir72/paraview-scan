/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProvider.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProvider.h"

#include "vtkObjectFactory.h"
#include "vtkService.h"

//----------------------------------------------------------------------------
vtkProvider::vtkProvider() = default;

//----------------------------------------------------------------------------
vtkProvider::~vtkProvider() = default;

//----------------------------------------------------------------------------
void vtkProvider::Initialize(vtkService* service)
{
  if (service == nullptr)
  {
    vtkErrorMacro("Service cannot be nullptr!");
  }
  else if (this->Initialized)
  {
    vtkErrorMacro("Provider was already initialized once!");
  }
  else
  {
    this->Initialized = true;
    this->Service = service;
    this->InitializeInternal(service);
  }
}

//----------------------------------------------------------------------------
vtkService* vtkProvider::GetService() const
{
  return this->Service;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkProvider::GetController() const
{
  return this->GetService()->GetController();
}

//----------------------------------------------------------------------------
void vtkProvider::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Service: " << this->Service << endl;
}
