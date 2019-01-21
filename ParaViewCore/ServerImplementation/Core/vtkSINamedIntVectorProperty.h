/*=========================================================================

  Program:   ParaView
  Module:    vtkSINamedIntVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSINamedIntVectorProperty
 *
 * ServerSide Property use to set int array as method argument.
*/

#ifndef vtkSINamedIntVectorProperty_h
#define vtkSINamedIntVectorProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSINamedVectorPropertyTemplate.h"    // real superclass
#include "vtkSIVectorProperty.h"

#ifndef __WRAP__
#define vtkSIVectorProperty vtkSINamedVectorPropertyTemplate<int>
#endif
class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSINamedIntVectorProperty : public vtkSIVectorProperty
#ifndef __WRAP__
#undef vtkSIVectorProperty
#endif
{
public:
  static vtkSINamedIntVectorProperty* New();
  vtkTypeMacro(vtkSINamedIntVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSINamedIntVectorProperty();
  ~vtkSINamedIntVectorProperty() override;

private:
  vtkSINamedIntVectorProperty(const vtkSINamedIntVectorProperty&) = delete;
  void operator=(const vtkSINamedIntVectorProperty&) = delete;
};

#endif
