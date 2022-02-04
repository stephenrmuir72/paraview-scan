/*=========================================================================

  Program:   ParaView
  Module:    vtkServerSessionViews.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkServerSessionViews.h"

#include "vtkInteractionProvider.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVViewProvider.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"

#include <vector>

class vtkServerSessionViews::vtkInternals
{
public:
  std::vector<vtkSmartPointer<vtkProvider>> Providers;
};

vtkObjectFactoryNewMacro(vtkServerSessionViews);
//----------------------------------------------------------------------------
vtkServerSessionViews::vtkServerSessionViews()
  : Internals(new vtkServerSessionViews::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkServerSessionViews::~vtkServerSessionViews() = default;

//----------------------------------------------------------------------------
void vtkServerSessionViews::InitializeService(vtkService* service, vtkObjectStore* store)
{
  this->Superclass::InitializeService(service, store);

  auto& internals = (*this->Internals);

  if (service->GetName() == "rs")
  {
    vtkLogF(INFO, "creating vtkInteractionProvider");
    auto provider = vtk::TakeSmartPointer(vtkInteractionProvider::New());
    provider->SetObjectStore(store);
    provider->Initialize(service);
    internals.Providers.emplace_back(provider);
  }

  vtkLogF(INFO, "creating vtkPVViewProvider");
  auto viewProvider = vtk::TakeSmartPointer(vtkPVViewProvider::New());
  viewProvider->SetObjectStore(store);
  viewProvider->Initialize(service);
  internals.Providers.emplace_back(viewProvider);
}

//----------------------------------------------------------------------------
void vtkServerSessionViews::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
