/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMaterialDomain.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMMaterialDomain);
//---------------------------------------------------------------------------
vtkSMMaterialDomain::vtkSMMaterialDomain() = default;

//---------------------------------------------------------------------------
vtkSMMaterialDomain::~vtkSMMaterialDomain() = default;

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
int vtkSMMaterialDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  if (auto* mLibrary = controller->FindMaterialLibrary(prop->GetSession()))
  {
    if (auto* prop = mLibrary->GetProperty("Materials"))
    {
      prop->AddObserver(vtkCommand::ModifiedEvent, this, &vtkSMMaterialDomain::MaterialsUpdated);
      this->Update(prop);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::MaterialsUpdated(vtkObject* sender, unsigned long, void*)
{
  this->Update(vtkSMProperty::SafeDownCast(sender));
}

//---------------------------------------------------------------------------
void vtkSMMaterialDomain::Update(vtkSMProperty* prop)
{
  auto* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
  {
    return;
  }

  std::vector<std::string> values;
  values.push_back(std::string("None")); // standard vtk coloration
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  values.push_back("Value Indexed"); // cells/blocks can choose for themselves.

  const auto& elements = svp->GetElements();
  std::copy(elements.begin(), elements.end(), std::back_inserter(values));
#endif
  this->SetStrings(values);
}
