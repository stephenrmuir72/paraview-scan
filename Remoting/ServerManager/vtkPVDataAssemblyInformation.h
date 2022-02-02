/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataAssemblyInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVDataAssemblyInformation
 * @brief fetches vtkDataAssembly from a vtkObject subclass
 *
 * vtkPVDataAssemblyInformation is used to fetch vtkDataAssembly from a
 * vtkObject. The method used to obtain vtkDataAssembly instance is defined by
 * `MethodName` and defaults to "GetAssembly".
 */

#ifndef vtkPVDataAssemblyInformation_h
#define vtkPVDataAssemblyInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

class vtkDataAssembly;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVDataAssemblyInformation : public vtkPVInformation
{
public:
  static vtkPVDataAssemblyInformation* New();
  vtkTypeMacro(vtkPVDataAssemblyInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the method name to use to get the assembly.
   */
  const std::string& GetMethodName() const;
  void SetMethodName(const std::string& name);
  ///@}

  vtkGetObjectMacro(DataAssembly, vtkDataAssembly);

  ///@{
  bool GatherInformation(vtkObject* target) override;
  void AddInformation(vtkPVInformation*) override {}
  vtkJson SaveState() const override;
  bool LoadState(const vtkJson& state) override;
  bool LoadInformation(const vtkJson& json) override;
  vtkJson SaveInformation() const override;
  ///@}

protected:
  vtkPVDataAssemblyInformation();
  ~vtkPVDataAssemblyInformation() override;

private:
  vtkPVDataAssemblyInformation(const vtkPVDataAssemblyInformation&) = delete;
  void operator=(const vtkPVDataAssemblyInformation&) = delete;

  void SetDataAssembly(vtkDataAssembly*);

  vtkDataAssembly* DataAssembly;
  std::string MethodName;
};

#endif
