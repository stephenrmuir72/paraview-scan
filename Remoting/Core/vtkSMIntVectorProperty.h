/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMIntVectorProperty
 * @brief
 *
 */

#ifndef vtkSMIntVectorProperty_h
#define vtkSMIntVectorProperty_h

#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkSMPropertyTemplate.h"

class VTKREMOTINGCORE_EXPORT vtkSMIntVectorProperty : public vtkSMPropertyTemplate<int>
{
public:
  static vtkSMIntVectorProperty* New();
  vtkTypeMacro(vtkSMIntVectorProperty, vtkSMPropertyTemplate);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns an unique identifier for the property's type.
   */
  int GetElementType() const override { return vtkSMProperty::INT; }

protected:
  vtkSMIntVectorProperty();
  ~vtkSMIntVectorProperty();

  int ToValue(const std::string& valueString) const override;
  vtkJson ToJson(const int& value) const override;
  int ToValue(const vtkJson& json, vtkSMStateContext* context) const override;

private:
  vtkSMIntVectorProperty(const vtkSMIntVectorProperty&) = delete;
  void operator=(const vtkSMIntVectorProperty&) = delete;
};

#endif
