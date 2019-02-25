/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNamedStringVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMNamedStringVectorProperty
 * @brief   property representing a vector of doubles
 *
 * vtkSMNamedStringVectorProperty is a concrete sub-class of vtkSMVectorProperty
 * representing a vector of doubles.
 * @sa
 * vtkSMVectorProperty vtkSMIntVectorProperty vtkSMStringVectorProperty
*/
#ifndef vtkSMNamedStringVectorProperty_h
#define vtkSMNamedStringVectorProperty_h

#include "vtkSMStringVectorProperty.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMNamedStringVectorProperty
  : public vtkSMStringVectorProperty
{
public:
  static vtkSMNamedStringVectorProperty* New();
  vtkTypeMacro(vtkSMNamedStringVectorProperty, vtkSMStringVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetStringMacro(PropertyName);
  vtkSetStringMacro(PropertyName);

protected:
  vtkSMNamedStringVectorProperty();
  ~vtkSMNamedStringVectorProperty() override;

  int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element) override;
  void Copy(vtkSMProperty* src) override;

  char* PropertyName;
};

#endif
