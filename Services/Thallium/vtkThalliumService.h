/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumService.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkThalliumService
 * @brief
 *
 */

#ifndef vtkThalliumService_h
#define vtkThalliumService_h

#include "vtkService.h"
#include "vtkServicesThalliumModule.h" // for exports

class VTKSERVICESTHALLIUM_EXPORT vtkThalliumService : public vtkService
{
public:
  static vtkThalliumService* New();
  vtkTypeMacro(vtkThalliumService, vtkService);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkThalliumService(const vtkThalliumService&) = delete;
  vtkThalliumService& operator=(const vtkThalliumService&) = delete;

  ///@{
  /**
   * Implementation of the vtkService API.
   */
  const rxcpp::schedulers::run_loop& GetRunLoop() const override;
  ///@}

protected:
  vtkThalliumService();
  ~vtkThalliumService() override;

  void InitializeInternal() override;
  bool StartInternal() override;
  void ShutdownInternal() override;

private:
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  class vtkProvider;
  friend class vtkInternals;
  friend class vtkProvider;
};

#endif
