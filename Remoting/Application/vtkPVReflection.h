/*=========================================================================

  Program:   ParaView
  Module:    vtkPVReflection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVReflection
 * @brief internal class
 *
 * vtkPVReflection is used to initialize reflection for all class that have been
 * wrapped in the build. It uses object-factor override mechanisms to override
 * vtkReflectionInitializer. vtkReflectionInitializer is instantiated in
 * vtkPVCoreApplication to ensure wrapping is initialized before the application
 * starts creating proxies etc.
 *
 * One does not need to use this class directly.
 */

#ifndef vtkPVReflection_h
#define vtkPVReflection_h

#include "vtkReflectionInitializer.h"
#include "vtkRemotingApplicationModule.h" // for exports

class VTKREMOTINGAPPLICATION_EXPORT vtkPVReflection : public vtkReflectionInitializer
{
public:
  static vtkPVReflection* New();
  vtkTypeMacro(vtkPVReflection, vtkReflectionInitializer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize() override;

protected:
  vtkPVReflection();
  ~vtkPVReflection() override;

private:
  vtkPVReflection(const vtkPVReflection&) = delete;
  void operator=(const vtkPVReflection&) = delete;
};

#endif
