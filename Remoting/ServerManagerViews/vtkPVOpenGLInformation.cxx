/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOpenGLInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOpenGLInformation.h"

#include "vtkObjectFactory.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVView.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPVOpenGLInformation);
//----------------------------------------------------------------------------
vtkPVOpenGLInformation::vtkPVOpenGLInformation()
{
  this->SetRootOnly(true);
  this->Vendor = this->Version = this->Renderer = this->Capabilities = "Information Unavailable";
}

//----------------------------------------------------------------------------
vtkPVOpenGLInformation::~vtkPVOpenGLInformation() = default;

//-----------------------------------------------------------------------------
bool vtkPVOpenGLInformation::GatherInformation(vtkObject* obj)
{
  this->Vendor = this->Version = this->Renderer = this->Capabilities = "Information Unavailable";

  vtkPVView* view = vtkPVView::SafeDownCast(obj);
  vtkSmartPointer<vtkRenderWindow> renWin =
    view ? view->GetRenderWindow() : vtkRenderWindow::SafeDownCast(obj);
  if (renWin == nullptr)
  {
    // create new one based on current process's capabilities.
    renWin = vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();

    // ensure context is created and OpenGL initialized.
    renWin->Render();
  }

  if (const char* opengl_capabilities = renWin->ReportCapabilities())
  {
    vtksys::RegularExpression reVendor("OpenGL vendor string:[ ]*([^\n\r]*)");
    if (reVendor.find(opengl_capabilities))
    {
      this->Vendor = reVendor.match(1);
    }

    vtksys::RegularExpression reVersion("OpenGL version string:[ ]*([^\n\r]*)");
    if (reVersion.find(opengl_capabilities))
    {
      this->Version = reVersion.match(1);
    }

    vtksys::RegularExpression reRenderer("OpenGL renderer string:[ ]*([^\n\r]*)");
    if (reRenderer.find(opengl_capabilities))
    {
      this->Renderer = reRenderer.match(1);
    }

    this->Capabilities = opengl_capabilities;
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLInformation::AddInformation(vtkPVInformation* pvinfo)
{
  if (!pvinfo)
  {
    return;
  }

  vtkPVOpenGLInformation* info = vtkPVOpenGLInformation::SafeDownCast(pvinfo);
  if (!info)
  {
    vtkErrorMacro("Could not downcast to vtkPVOpenGLInformation.");
    return;
  }
  this->Vendor = info->Vendor;
  this->Version = info->Version;
  this->Renderer = info->Renderer;
  this->Capabilities = info->Capabilities;
}

//-----------------------------------------------------------------------------
vtkNJson vtkPVOpenGLInformation::SaveInformation() const
{
  vtkNJson json;
  VTK_NJSON_SAVE_MEMBER(json, Vendor);
  VTK_NJSON_SAVE_MEMBER(json, Version);
  VTK_NJSON_SAVE_MEMBER(json, Renderer);
  VTK_NJSON_SAVE_MEMBER(json, Capabilities);
  return json;
}

//-----------------------------------------------------------------------------
bool vtkPVOpenGLInformation::LoadInformation(const vtkNJson& json)
{
  VTK_NJSON_LOAD_MEMBER(json, Vendor);
  VTK_NJSON_LOAD_MEMBER(json, Version);
  VTK_NJSON_LOAD_MEMBER(json, Renderer);
  VTK_NJSON_LOAD_MEMBER(json, Capabilities);
  return true;
}

//----------------------------------------------------------------------------
void vtkPVOpenGLInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
