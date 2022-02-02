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
/**
 * @class vtkPVSystemConfigInformation
 * @brief Information object providing system resource information
 *
 * vtkPVSystemConfigInformation is intended to collect information about system
 * resources such as memory used, available etc.
 */

#ifndef vtkPVSystemConfigInformation_h
#define vtkPVSystemConfigInformation_h

#include "vtkPVInformation.h"

#include <string> // for std::string
#include <vector> // for std::vector

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVSystemConfigInformation : public vtkPVInformation
{
public:
  class ConfigInfo
  {
  public:
    ConfigInfo()
      : ProcessType(-1)
      , SystemType(-1)
      , Rank(-1)
      , Pid(0)
      , HostMemoryTotal(0)
      , HostMemoryAvailable(0)
      , ProcMemoryAvailable(0)
    {
    }

    void Print();

    bool operator<(const ConfigInfo& other) const { return this->Rank < other.Rank; }

    std::string OSDescriptor;
    std::string CPUDescriptor;
    std::string MemDescriptor;
    std::string HostName;
    int ProcessType;
    int SystemType;
    int Rank;
    long long Pid;
    long long HostMemoryTotal;
    long long HostMemoryAvailable;
    long long ProcMemoryAvailable;
  };

  static vtkPVSystemConfigInformation* New();
  vtkTypeMacro(vtkPVSystemConfigInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * vtkPVInformation overrides.
   */
  bool GatherInformation(vtkObject* target) override;
  void AddInformation(vtkPVInformation* info) override;
  bool LoadInformation(const vtkJson& json) override;
  vtkJson SaveInformation() const override;
  ///@}

  ///@{
  /**
   * Access managed information
   */
  size_t GetSize() { return this->Configs.size(); }
  const char* GetOSDescriptor(size_t i) { return this->Configs[i].OSDescriptor.c_str(); }
  const char* GetCPUDescriptor(size_t i) { return this->Configs[i].CPUDescriptor.c_str(); }
  const char* GetMemoryDescriptor(size_t i) { return this->Configs[i].MemDescriptor.c_str(); }
  const char* GetHostName(size_t i) { return this->Configs[i].HostName.c_str(); }
  int GetProcessType(size_t i) { return this->Configs[i].ProcessType; }
  int GetSystemType(size_t i) { return this->Configs[i].SystemType; }
  int GetRank(size_t i) { return this->Configs[i].Rank; }
  long long GetPid(size_t i) { return this->Configs[i].Pid; }
  long long GetHostMemoryTotal(size_t i) { return this->Configs[i].HostMemoryTotal; }
  long long GetHostMemoryAvailable(size_t i) { return this->Configs[i].HostMemoryAvailable; }
  long long GetProcMemoryAvailable(size_t i) { return this->Configs[i].ProcMemoryAvailable; }
  ///@}

  /**
   * Sort elements by mpi rank.
   */
  void Sort();

protected:
  vtkPVSystemConfigInformation();
  ~vtkPVSystemConfigInformation() override;

private:
  std::vector<ConfigInfo> Configs;

  vtkPVSystemConfigInformation(const vtkPVSystemConfigInformation&) = delete;
  void operator=(const vtkPVSystemConfigInformation&) = delete;
};

#endif
