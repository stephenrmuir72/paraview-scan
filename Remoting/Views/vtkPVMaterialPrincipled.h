/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterialPrincipled.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMaterialPrincipled
 * @brief   manages material definitions
 *
 * vtkPVMaterialPrincipled helps ParaView to keep track of material instances.
*/

#ifndef vtkPVMaterialPrincipled_h
#define vtkPVMaterialPrincipled_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

#include "vtkPVMaterial.h"
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVMaterialPrincipled : public vtkPVMaterial
{
public:
  static vtkPVMaterialPrincipled* New();
  vtkTypeMacro(vtkPVMaterialPrincipled, vtkPVMaterial);

protected:
  vtkPVMaterialPrincipled() = default;
  ~vtkPVMaterialPrincipled() override = default;

private:
  vtkPVMaterialPrincipled(const vtkPVMaterialPrincipled&) = delete;
  void operator=(const vtkPVMaterialPrincipled&) = delete;
};

#endif
