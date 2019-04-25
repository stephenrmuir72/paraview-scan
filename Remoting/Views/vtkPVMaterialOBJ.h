/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterialOBJ.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMaterialOBJ
 * @brief   manages material definitions
 *
 * vtkPVMaterialOBJ helps ParaView to keep track of material instances.
*/

#ifndef vtkPVMaterialOBJ_h
#define vtkPVMaterialOBJ_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

#include "vtkPVMaterial.h"
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVMaterialOBJ : public vtkPVMaterial
{
public:
  static vtkPVMaterialOBJ* New();
  vtkTypeMacro(vtkPVMaterialOBJ, vtkPVMaterial);

  void SetDiffuse(double r, double g, double b);

  void SetSpecular(double r, double g, double b);

  void SetShininess(double shininess);

  void SetOpacity(double opacity);

  void SetTransparencyFilter(double r, double g, double b);

protected:
  vtkPVMaterialOBJ() = default;
  ~vtkPVMaterialOBJ() override = default;

private:
  vtkPVMaterialOBJ(const vtkPVMaterialOBJ&) = delete;
  void operator=(const vtkPVMaterialOBJ&) = delete;
};

#endif
