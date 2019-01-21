/*=========================================================================

  Program:   ParaView
  Module:    vtkSINamedDoubleVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSINamedDoubleVectorProperty
 *
 * Vector property that manage double value to be set through a method
 * on a vtkObject.
*/

#ifndef vtkSINamedDoubleVectorProperty_h
#define vtkSINamedDoubleVectorProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSINamedVectorPropertyTemplate.h"    // real superclass
#include "vtkSIVectorProperty.h"

#ifndef __WRAP__
#define vtkSIVectorProperty vtkSINamedVectorPropertyTemplate<double>
#endif
class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSINamedDoubleVectorProperty
  : public vtkSIVectorProperty
#ifndef __WRAP__
#undef vtkSIVectorProperty
#endif
{
public:
  static vtkSINamedDoubleVectorProperty* New();
  vtkTypeMacro(vtkSINamedDoubleVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSINamedDoubleVectorProperty();
  ~vtkSINamedDoubleVectorProperty() override;

private:
  vtkSINamedDoubleVectorProperty(const vtkSINamedDoubleVectorProperty&) = delete;
  void operator=(const vtkSINamedDoubleVectorProperty&) = delete;
};

#endif
