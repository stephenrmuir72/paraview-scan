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
 * @brief
 *
 */

#ifndef vtkPVReflection_h
#define vtkPVReflection_h

#include "vtkReflectionInitializer.h"
#include "vtkServerManagerInitializationModule.h" // for exports

class VTKSERVERMANAGERINITIALIZATION_EXPORT vtkPVReflection : public vtkReflectionInitializer
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
