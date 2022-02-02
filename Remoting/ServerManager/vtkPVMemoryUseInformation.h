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
 * @class   vtkPVMemoryUseInformation
 *
 */

#ifndef vtkPVMemoryUseInformation_h
#define vtkPVMemoryUseInformation_h

#include "vtkPVInformation.h"

#include <vector> // needed for std::vector
using std::vector;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVMemoryUseInformation : public vtkPVInformation
{
public:
  static vtkPVMemoryUseInformation* New();
  vtkTypeMacro(vtkPVMemoryUseInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool GatherInformation(vtkObject*) override;
  void AddInformation(vtkPVInformation*) override;
  bool LoadInformation(const vtkJson& json) override;
  vtkJson SaveInformation() const override;

  /**
   * access the managed information.
   */
  size_t GetSize() { return this->MemInfos.size(); }
  int GetProcessType(size_t i) { return this->MemInfos[i].ProcessType; }
  int GetRank(size_t i) { return this->MemInfos[i].Rank; }
  long long GetProcMemoryUse(size_t i) { return this->MemInfos[i].ProcMemUse; }
  long long GetHostMemoryUse(size_t i) { return this->MemInfos[i].HostMemUse; }

protected:
  vtkPVMemoryUseInformation();
  ~vtkPVMemoryUseInformation() override;

private:
  class MemInfo
  {
  public:
    MemInfo()
      : ProcessType(-1)
      , Rank(0)
      , ProcMemUse(0)
      , HostMemUse(0)
    {
    }
    void Print();

    int ProcessType;
    int Rank;
    long long ProcMemUse;
    long long HostMemUse;
  };
  vector<MemInfo> MemInfos;

  vtkPVMemoryUseInformation(const vtkPVMemoryUseInformation&) = delete;
  void operator=(const vtkPVMemoryUseInformation&) = delete;
};

#endif
