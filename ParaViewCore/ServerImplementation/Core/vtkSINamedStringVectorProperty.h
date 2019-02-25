/*=========================================================================

  Program:   ParaView
  Module:    vtkSINamedStringVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSINamedStringVectorProperty
 *
 * ServerImplementation Property to deal with String array as method arguments.
 * It also takes care of string encoding on server side.
*/

#ifndef vtkSINamedStringVectorProperty_h
#define vtkSINamedStringVectorProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIVectorProperty.h"

#include <string>

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSINamedStringVectorProperty
  : public vtkSIVectorProperty
{
public:
  static vtkSINamedStringVectorProperty* New();
  vtkTypeMacro(vtkSINamedStringVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSINamedStringVectorProperty();
  ~vtkSINamedStringVectorProperty() override;

  enum ElementTypes
  {
    INT,
    DOUBLE,
    STRING
  };

  /**
   * Push a new state to the underneath implementation
   */
  bool Push(vtkSMMessage*, int) VTK_OVERRIDE;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) VTK_OVERRIDE;

  /**
   * Parse the xml for the property.
   */
  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) VTK_OVERRIDE;

private:
  vtkSINamedStringVectorProperty(const vtkSINamedStringVectorProperty&) = delete;
  void operator=(const vtkSINamedStringVectorProperty&) = delete;

  class vtkVectorOfStrings;
  class vtkVectorOfInts;

  bool Push(const vtkVectorOfStrings& values);
  vtkVectorOfInts* ElementTypes;
  bool NeedReencoding;
};

#endif
