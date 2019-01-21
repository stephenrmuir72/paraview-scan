/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNamedDoubleVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMNamedDoubleVectorProperty
 * @brief   property representing a vector of doubles
 *
 * vtkSMNamedDoubleVectorProperty is a concrete sub-class of vtkSMVectorProperty
 * representing a vector of doubles.
 * @sa
 * vtkSMVectorProperty vtkSMIntVectorProperty vtkSMStringVectorProperty
*/
#ifndef vtkSMNamedDoubleVectorProperty_h
#define vtkSMNamedDoubleVectorProperty_h

#include "vtkSMDoubleVectorProperty.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMNamedDoubleVectorProperty : public vtkSMDoubleVectorProperty
{
public:
  static vtkSMNamedDoubleVectorProperty* New();
  vtkTypeMacro(vtkSMNamedDoubleVectorProperty, vtkSMDoubleVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetStringMacro(PropertyName);
  vtkSetStringMacro(PropertyName);

protected:
  vtkSMNamedDoubleVectorProperty();
  ~vtkSMNamedDoubleVectorProperty() override;

  int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element) override;
  void Copy(vtkSMProperty* src) override;

  char* PropertyName;
};

#endif
