/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIdTypeVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMIdTypeVectorProperty
 * @brief
 *
 */

#ifndef vtkSMIdTypeVectorProperty_h
#define vtkSMIdTypeVectorProperty_h

#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkSMPropertyTemplate.h"

class VTKREMOTINGCORE_EXPORT vtkSMIdTypeVectorProperty : public vtkSMPropertyTemplate<vtkIdType>
{
public:
  static vtkSMIdTypeVectorProperty* New();
  vtkTypeMacro(vtkSMIdTypeVectorProperty, vtkSMPropertyTemplate);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns an unique identifier for the property's type.
   */
  int GetElementType() const override { return vtkSMProperty::ID_TYPE; }

protected:
  vtkSMIdTypeVectorProperty();
  ~vtkSMIdTypeVectorProperty();

  vtkIdType ToValue(const std::string& valueString) const override;
  vtkJson ToJson(const vtkIdType& value) const override;
  vtkIdType ToValue(const vtkJson& json, vtkSMStateContext* context) const override;

private:
  vtkSMIdTypeVectorProperty(const vtkSMIdTypeVectorProperty&) = delete;
  void operator=(const vtkSMIdTypeVectorProperty&) = delete;
};

#endif
