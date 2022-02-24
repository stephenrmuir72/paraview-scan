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
#include "vtkPVSystemConfigInformation.h"

#include "vtkObjectFactory.h"
#include "vtkPVCoreApplication.h"
#include <vtksys/SystemInformation.hxx>

#include <algorithm>

void vtkPVSystemConfigInformation::ConfigInfo::Print()
{
  cerr << "OSDescriptor=" << this->OSDescriptor << endl
       << "CPUDescriptor=" << this->CPUDescriptor << endl
       << "MemDescriptor=" << this->MemDescriptor << endl
       << "HostName=" << this->HostName << endl
       << "ProcessType=" << this->ProcessType << endl
       << "SystemType=" << this->SystemType << endl
       << "Rank=" << this->Rank << endl
       << "Pid=" << this->Pid << endl
       << "HostMemoryTotal=" << this->HostMemoryTotal << endl
       << "HostMemoryAvailable=" << this->HostMemoryAvailable << endl
       << "ProcMemoryAvailable=" << this->ProcMemoryAvailable << endl
       << "ProcMemoryAvailable=" << this->ProcMemoryAvailable << endl;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSystemConfigInformation);

//----------------------------------------------------------------------------
vtkPVSystemConfigInformation::vtkPVSystemConfigInformation() = default;

//----------------------------------------------------------------------------
vtkPVSystemConfigInformation::~vtkPVSystemConfigInformation() = default;

//----------------------------------------------------------------------------
bool vtkPVSystemConfigInformation::GatherInformation(vtkObject* vtkNotUsed(obj))
{
  this->Configs.clear();

  auto* pvapp = vtkPVCoreApplication::GetInstance();

  ConfigInfo info;
  vtksys::SystemInformation sysInfo;
  sysInfo.RunCPUCheck();
  sysInfo.RunOSCheck();
  sysInfo.RunMemoryCheck();

  info.OSDescriptor = sysInfo.GetOSDescription();
  info.CPUDescriptor = sysInfo.GetCPUDescription();
  info.MemDescriptor = sysInfo.GetMemoryDescription();
  info.HostName = sysInfo.GetHostname();
  info.ProcessType = pvapp->GetApplicationType();
  info.SystemType = sysInfo.GetOSIsWindows();
  info.Rank = pvapp->GetRank();
  info.Pid = sysInfo.GetProcessId();
  info.HostMemoryTotal = sysInfo.GetHostMemoryTotal();
  // the folloowing envornment variables are querried, if set they
  // are the means of reporting limits that are applied in a non-
  // standard way.
  info.HostMemoryAvailable = sysInfo.GetHostMemoryAvailable("PV_HOST_MEMORY_LIMIT");
  info.ProcMemoryAvailable =
    sysInfo.GetProcMemoryAvailable("PV_HOST_MEMORY_LIMIT", "PV_PROC_MEMORY_LIMIT");

  this->Configs.push_back(info);
  return true;
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::AddInformation(vtkPVInformation* pvinfo)
{
  vtkPVSystemConfigInformation* info = vtkPVSystemConfigInformation::SafeDownCast(pvinfo);
  if (!info)
  {
    return;
  }

  std::copy(info->Configs.begin(), info->Configs.end(), std::back_inserter(this->Configs));
}

//----------------------------------------------------------------------------
vtkNJson vtkPVSystemConfigInformation::SaveInformation() const
{
  vtkNJson json;
  vtkNJson configs = vtkNJson::array();
  for (const auto& config : this->Configs)
  {
    vtkNJson item;
    VTK_JSON_SAVE_MEMBER(item, config.OSDescriptor);
    VTK_JSON_SAVE_MEMBER(item, config.CPUDescriptor);
    VTK_JSON_SAVE_MEMBER(item, config.MemDescriptor);
    VTK_JSON_SAVE_MEMBER(item, config.HostName);
    VTK_JSON_SAVE_MEMBER(item, config.ProcessType);
    VTK_JSON_SAVE_MEMBER(item, config.SystemType);
    VTK_JSON_SAVE_MEMBER(item, config.Rank);
    VTK_JSON_SAVE_MEMBER(item, config.Pid);
    VTK_JSON_SAVE_MEMBER(item, config.HostMemoryTotal);
    VTK_JSON_SAVE_MEMBER(item, config.HostMemoryAvailable);
    VTK_JSON_SAVE_MEMBER(item, config.ProcMemoryAvailable);
    configs.push_back(std::move(item));
  }
  json["configs"] = std::move(configs);
  return json;
}

//----------------------------------------------------------------------------
bool vtkPVSystemConfigInformation::LoadInformation(const vtkNJson& json)
{
  this->Configs.clear();
  for (const auto& item : json.at("configs"))
  {
    ConfigInfo config;
    VTK_JSON_LOAD_MEMBER(item, config.OSDescriptor);
    VTK_JSON_LOAD_MEMBER(item, config.CPUDescriptor);
    VTK_JSON_LOAD_MEMBER(item, config.MemDescriptor);
    VTK_JSON_LOAD_MEMBER(item, config.HostName);
    VTK_JSON_LOAD_MEMBER(item, config.ProcessType);
    VTK_JSON_LOAD_MEMBER(item, config.SystemType);
    VTK_JSON_LOAD_MEMBER(item, config.Rank);
    VTK_JSON_LOAD_MEMBER(item, config.Pid);
    VTK_JSON_LOAD_MEMBER(item, config.HostMemoryTotal);
    VTK_JSON_LOAD_MEMBER(item, config.HostMemoryAvailable);
    VTK_JSON_LOAD_MEMBER(item, config.ProcMemoryAvailable);
    this->Configs.push_back(std::move(config));
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::Sort()
{
  std::sort(this->Configs.begin(), this->Configs.end());
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
