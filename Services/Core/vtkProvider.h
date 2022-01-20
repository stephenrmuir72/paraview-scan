/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProvider.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkProvider
 * @brief abstract base-class for all micro-services.
 *
 * In the VTK::Remoting module, providers can be thought of as micro-services
 * that add specific capability to a service. vtkProvider is intended to be the
 * base-class of all such micro-services.
 *
 */

#ifndef vtkProvider_h
#define vtkProvider_h

#include "vtkObject.h"
#include "vtkServicesCoreModule.h" // for exports
#include "vtkSmartPointer.h"       // for vtkSmartPointer

class vtkMultiProcessController;
class vtkService;

class VTKSERVICESCORE_EXPORT vtkProvider : public vtkObject
{
public:
  vtkTypeMacro(vtkProvider, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkProvider(const vtkProvider&) = delete;
  vtkProvider& operator=(const vtkProvider&) = delete;

  /**
   * Get the service to which this provider is attached.
   */
  vtkService* GetService() const;

  ///@{
  /**
   * Get the controller to use to reduce information in a distributed
   * environment.
   *
   * @sa vtkService::GetController
   */
  vtkMultiProcessController* GetController() const;
  ///@}

  /**
   * Called to initialize the provider on a service. Subclasses should override
   * `InitializeInternal` method to perform the initialization.
   */
  void Initialize(vtkService* service);

protected:
  vtkProvider();
  ~vtkProvider() override;

  virtual void InitializeInternal(vtkService* service) = 0;

private:
  vtkSmartPointer<vtkService> Service;
  bool Initialized = false;
};

#endif
