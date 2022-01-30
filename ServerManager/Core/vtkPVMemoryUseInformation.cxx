/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMemoryUseInformation.h"

#include "vtkObjectFactory.h"
#include "vtkPVCoreApplication.h"
#include <vtksys/SystemInformation.hxx>

vtkStandardNewMacro(vtkPVMemoryUseInformation);
vtkPVMemoryUseInformation::vtkPVMemoryUseInformation() = default;
vtkPVMemoryUseInformation::~vtkPVMemoryUseInformation() = default;

//----------------------------------------------------------------------------
bool vtkPVMemoryUseInformation::GatherInformation(vtkObject*)
{
  this->MemInfos.clear();
  vtksys::SystemInformation sysInfo;

  auto* pvapp = vtkPVCoreApplication::GetInstance();
  MemInfo info;
  info.ProcessType = pvapp->GetApplicationType();
  info.Rank = pvapp->GetRank();
  info.ProcMemUse = sysInfo.GetProcMemoryUsed();
  info.HostMemUse = sysInfo.GetHostMemoryUsed();
  this->MemInfos.push_back(info);
  return true;
}

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::AddInformation(vtkPVInformation* pvinfo)
{
  vtkPVMemoryUseInformation* info = vtkPVMemoryUseInformation::SafeDownCast(pvinfo);
  if (!info)
  {
    return;
  }
  std::copy(info->MemInfos.begin(), info->MemInfos.end(), std::back_inserter(this->MemInfos));
}

//----------------------------------------------------------------------------
vtkJson vtkPVMemoryUseInformation::SaveInformation() const
{
  vtkJson json = vtkJson::array();
  for (const auto& info : MemInfos)
  {
    vtkJson item;
    VTK_JSON_SAVE_MEMBER(item, info.ProcessType);
    VTK_JSON_SAVE_MEMBER(item, info.Rank);
    VTK_JSON_SAVE_MEMBER(item, info.ProcMemUse);
    VTK_JSON_SAVE_MEMBER(item, info.HostMemUse);
    json.push_back(std::move(item));
  }
  return json;
}

//----------------------------------------------------------------------------
bool vtkPVMemoryUseInformation::LoadInformation(const vtkJson& json)
{
  this->MemInfos.clear();
  for (const auto& item : json)
  {
    MemInfo info;
    VTK_JSON_LOAD_MEMBER(item, info.ProcessType);
    VTK_JSON_LOAD_MEMBER(item, info.Rank);
    VTK_JSON_LOAD_MEMBER(item, info.ProcMemUse);
    VTK_JSON_LOAD_MEMBER(item, info.HostMemUse);
    this->MemInfos.push_back(std::move(info));
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::MemInfo::Print()
{
  cerr << "ProcessType=" << this->ProcessType << endl
       << "Rank=" << this->Rank << endl
       << "ProcMemUse=" << this->ProcMemUse << endl
       << "HostMemUse=" << this->HostMemUse << endl;
}
