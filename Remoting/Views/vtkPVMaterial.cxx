/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterial.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMaterial.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayMaterialLibrary.h"
#endif

#include <sstream>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMaterial);

//-----------------------------------------------------------------------------
void vtkPVMaterial::SetName(const std::string& name)
{
  this->Name = name;
  this->Modified();
}

//-----------------------------------------------------------------------------
const std::string& vtkPVMaterial::GetName()
{
  return this->Name;
}

//-----------------------------------------------------------------------------
void vtkPVMaterial::SetType(const std::string& type)
{
  this->Type = type;
  this->Modified();
}

//-----------------------------------------------------------------------------
const std::string& vtkPVMaterial::GetType()
{
  return this->Type;
}

//-----------------------------------------------------------------------------
void vtkPVMaterial::AddVariable(const char* paramName, const char* value)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayMaterialLibrary* lib = vtkOSPRayMaterialLibrary::SafeDownCast(this->Library);
  if (lib)
  {
    auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
    auto paramType = dic.at(lib->LookupImplName(this->Name)).at(paramName);

    if (paramType == vtkOSPRayMaterialLibrary::ParameterType::TEXTURE)
    {
      // get texture pointer
      vtkTexture* texture = nullptr;
      lib->AddTexture(this->Name, paramName, texture);
    }
    else
    {
      std::vector<double> data;
      std::stringstream ss(value);
      double current;
      while (ss >> current)
      {
        data.push_back(current);
      }
      lib->AddShaderVariable(this->Name, paramName, data.size(), data.data());
    }

    this->Modified();
  }
#else
  (void)paramName;
  (void)value;
#endif
}

//-----------------------------------------------------------------------------
void vtkPVMaterial::RemoveAllVariables()
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayMaterialLibrary* lib = vtkOSPRayMaterialLibrary::SafeDownCast(this->Library);
  if (lib)
  {
    lib->RemoveAllShaderVariables(this->Name);
    lib->RemoveAllTextures(this->Name);
    this->Modified();
  }
#endif
}

//-----------------------------------------------------------------------------
void vtkPVMaterial::SetLibrary(vtkObject* lib)
{
  this->Library = lib;
}
