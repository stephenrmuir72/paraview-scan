/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNamedIntVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMNamedIntVectorProperty
 * @brief   property representing a vector of doubles
 *
 * vtkSMNamedIntVectorProperty is a concrete sub-class of vtkSMVectorProperty
 * representing a vector of doubles.
 * @sa
 * vtkSMVectorProperty vtkSMIntVectorProperty vtkSMStringVectorProperty
*/
#ifndef vtkSMNamedIntVectorProperty_h
#define vtkSMNamedIntVectorProperty_h

#include "vtkSMIntVectorProperty.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMNamedIntVectorProperty : public vtkSMIntVectorProperty
{
public:
  static vtkSMNamedIntVectorProperty* New();
  vtkTypeMacro(vtkSMNamedIntVectorProperty, vtkSMIntVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetStringMacro(PropertyName);
  vtkSetStringMacro(PropertyName);

protected:
  vtkSMNamedIntVectorProperty();
  ~vtkSMNamedIntVectorProperty() override;

  int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element) override;
  void Copy(vtkSMProperty* src) override;

  char* PropertyName;
};

#endif
