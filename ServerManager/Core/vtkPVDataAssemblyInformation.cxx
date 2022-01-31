/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataAssemblyInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataAssemblyInformation.h"

#include "vtkDataAssembly.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkReflection.h"

vtkStandardNewMacro(vtkPVDataAssemblyInformation);
vtkCxxSetObjectMacro(vtkPVDataAssemblyInformation, DataAssembly, vtkDataAssembly);
//----------------------------------------------------------------------------
vtkPVDataAssemblyInformation::vtkPVDataAssemblyInformation()
  : DataAssembly(nullptr)
  , MethodName("GetAssembly")
{
  this->SetRootOnly(true);
}

//----------------------------------------------------------------------------
vtkPVDataAssemblyInformation::~vtkPVDataAssemblyInformation()
{
  this->SetDataAssembly(nullptr);
}

//----------------------------------------------------------------------------
const std::string& vtkPVDataAssemblyInformation::GetMethodName() const
{
  return this->MethodName;
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::SetMethodName(const std::string& name)
{
  this->MethodName = name;
}

//----------------------------------------------------------------------------
vtkJson vtkPVDataAssemblyInformation::SaveState() const
{
  auto json = this->Superclass::SaveState();
  VTK_JSON_SAVE_MEMBER(json, MethodName);
  return json;
}

//----------------------------------------------------------------------------
bool vtkPVDataAssemblyInformation::LoadState(const vtkJson& state)
{
  if (!this->Superclass::LoadState(state))
  {
    return false;
  }
  VTK_JSON_LOAD_MEMBER(state, MethodName);
  return true;
}

//----------------------------------------------------------------------------
vtkJson vtkPVDataAssemblyInformation::SaveInformation() const
{
  vtkJson json;
  if (this->DataAssembly)
  {
    json["DataAssembly"] = this->DataAssembly->SerializeToXML(vtkIndent());
  }
  else
  {
    json["DataAssembly"] = std::string();
  }
  return json;
}

//----------------------------------------------------------------------------
bool vtkPVDataAssemblyInformation::LoadInformation(const vtkJson& json)
{
  if (json.find("DataAssembly") == json.end())
  {
    return false;
  }

  this->SetDataAssembly(nullptr);

  auto xml = json.at("DataAssembly").get<std::string>();
  if (!xml.empty())
  {
    vtkNew<vtkDataAssembly> info;
    info->InitializeFromXML(xml.c_str());
    this->SetDataAssembly(info);
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVDataAssemblyInformation::GatherInformation(vtkObject* obj)
{
  this->SetDataAssembly(nullptr);
  std::vector<vtkVariant> args;
  if (vtkReflection::Get(obj, this->MethodName, args) && args.size() == 1)
  {
    if (auto assembly = vtk::variant_cast_to_VTKObject<vtkDataAssembly*>(args[0]))
    {
      // we store a clone just to avoid the scenario where the `obj` modifies
      // the vtkDataAssembly under the covers.
      vtkNew<vtkDataAssembly> clone;
      clone->DeepCopy(assembly);
      this->SetDataAssembly(clone);
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPVDataAssemblyInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataAssembly: " << this->DataAssembly << endl;
  os << indent << "MethodName: " << this->MethodName << endl;
}
