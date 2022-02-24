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
  bool RootOnly{ false };
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
void vtkPVInformation::SetRootOnly(bool rootOnly)
{
  auto& internals = (*this->Internals);
  internals.RootOnly = rootOnly;
}

//----------------------------------------------------------------------------
bool vtkPVInformation::GetRootOnly() const
{
  const auto& internals = (*this->Internals);
  return internals.RootOnly;
}

//----------------------------------------------------------------------------
bool vtkPVInformation::CanRunOnRPC() const
{
  return false;
}

//----------------------------------------------------------------------------
vtkNJson vtkPVInformation::SaveState() const
{
  return vtkNJson();
}

//----------------------------------------------------------------------------
bool vtkPVInformation::LoadState(const vtkNJson& /*state*/)
{
  return true;
}
