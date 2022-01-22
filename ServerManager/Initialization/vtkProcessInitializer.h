/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessInitializer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkProcessInitializer
 * @brief 
 */

#ifndef vtkProcessInitializer_h
#define vtkProcessInitializer_h
#include "vtkProcessModule.h"
#include "vtkServerManagerInitializationModule.h"

class VTKSERVERMANAGERINITIALIZATION_EXPORT vtkProcessInitializer : public vtkProcessModule
{
public:
  vtkTypeMacro(vtkProcessInitializer, vtkProcessModule);

  static vtkProcessInitializer* New();

protected:
  vtkProcessInitializer();
  ~vtkProcessInitializer() override;

private:
  vtkProcessInitializer(const vtkProcessInitializer&) = delete;
  void operator=(const vtkProcessInitializer&) = delete;
  void InitializeReflection() override;
};
#endif
