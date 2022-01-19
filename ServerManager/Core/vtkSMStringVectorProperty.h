/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMStringVectorProperty
 * @brief
 *
 */

#ifndef vtkSMStringVectorProperty_h
#define vtkSMStringVectorProperty_h

#include "vtkSMPropertyTemplate.h"
#include "vtkServerManagerCoreModule.h" //needed for exports

#include <string> // for std::string

class VTKSERVERMANAGERCORE_EXPORT vtkSMStringVectorProperty
  : public vtkSMPropertyTemplate<std::string>
{
public:
  static vtkSMStringVectorProperty* New();
  vtkTypeMacro(vtkSMStringVectorProperty, vtkSMPropertyTemplate);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns an unique identifier for the property's type.
   */
  int GetElementType() const override { return vtkSMProperty::STRING; }

protected:
  vtkSMStringVectorProperty();
  ~vtkSMStringVectorProperty();

  std::string ToValue(const std::string& valueString) const override;
  vtkJson ToJson(const std::string& value) const override;
  std::string ToValue(const vtkJson& json, vtkSMStateContext* context) const override;

private:
  vtkSMStringVectorProperty(const vtkSMStringVectorProperty&) = delete;
  void operator=(const vtkSMStringVectorProperty&) = delete;
};

#endif
