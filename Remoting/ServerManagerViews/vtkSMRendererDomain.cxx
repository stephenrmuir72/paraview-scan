/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRendererDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRendererDomain.h"

#include "vtkObjectFactory.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayPass.h"
#endif

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMRendererDomain);

//---------------------------------------------------------------------------
void vtkSMRendererDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
int vtkSMRendererDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  // throw away whatever XML said our strings are and call update instead
  this->Update(nullptr);
#endif
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMRendererDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
  // populate my list
  std::vector<std::string> sa;

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  if (vtkOSPRayPass::IsBackendAvailable("OSPRay raycaster"))
  {
    sa.push_back(std::string("OSPRay raycaster"));
  }
  if (vtkOSPRayPass::IsBackendAvailable("OSPRay pathtracer"))
  {
    sa.push_back(std::string("OSPRay pathtracer"));
  }
  if (vtkOSPRayPass::IsBackendAvailable("OptiX pathtracer"))
  {
    sa.push_back(std::string("OptiX pathtracer"));
  }
#endif
  this->SetStrings(sa);
}
