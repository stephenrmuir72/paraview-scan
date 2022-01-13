/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInformation.h"

#include "vtkMultiProcessController.h"
#include "vtkSmartPointer.h"

class vtkPVInformation::vtkInternals
{
public:
  vtkSmartPointer<vtkMultiProcessController> Controller;
};

//----------------------------------------------------------------------------
vtkPVInformation::vtkPVInformation()
  : Internals(new vtkPVInformation::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVInformation::~vtkPVInformation() = default;

//----------------------------------------------------------------------------
void vtkPVInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVInformation::SetController(vtkMultiProcessController* controller)
{
  auto& internals = (*this->Internals);
  internals.Controller = controller;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVInformation::GetController() const
{
  const auto& internals = (*this->Internals);
  return internals.Controller;
}

//----------------------------------------------------------------------------
vtkJson vtkPVInformation::SaveState() const
{
  return vtkJson();
}

//----------------------------------------------------------------------------
bool vtkPVInformation::LoadState(const vtkJson& /*state*/)
{
  return true;
}
