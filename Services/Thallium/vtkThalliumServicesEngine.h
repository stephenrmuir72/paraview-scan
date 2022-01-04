/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumServicesEngine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkThalliumServicesEngine
 * @brief
 *
 */

#ifndef vtkThalliumServicesEngine_h
#define vtkThalliumServicesEngine_h

#include "vtkServicesEngine.h"
#include "vtkServicesThalliumModule.h" // for exports

class vtkThalliumServicesEngineInternals;

class VTKSERVICESTHALLIUM_EXPORT vtkThalliumServicesEngine : public vtkServicesEngine
{
public:
  static vtkThalliumServicesEngine* New();
  vtkTypeMacro(vtkThalliumServicesEngine, vtkServicesEngine);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkThalliumServicesEngine(const vtkThalliumServicesEngine&) = delete;
  vtkThalliumServicesEngine& operator=(const vtkThalliumServicesEngine&) = delete;

protected:
  vtkThalliumServicesEngine();
  ~vtkThalliumServicesEngine() override;

  std::string InitializeInternal(const std::string& url) override;
  void FinalizeInternal() override;

  friend class vtkThalliumService;
  friend class vtkThalliumServiceEndpoint;
  vtkThalliumServicesEngineInternals* GetInternals();

private:
  std::unique_ptr<vtkThalliumServicesEngineInternals> Internals;
};

#endif
