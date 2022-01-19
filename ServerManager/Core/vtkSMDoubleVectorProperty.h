/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMDoubleVectorProperty
 * @brief
 *
 */

#ifndef vtkSMDoubleVectorProperty_h
#define vtkSMDoubleVectorProperty_h

#include "vtkSMPropertyTemplate.h"
#include "vtkServerManagerCoreModule.h" //needed for exports

class VTKSERVERMANAGERCORE_EXPORT vtkSMDoubleVectorProperty : public vtkSMPropertyTemplate<double>
{
public:
  static vtkSMDoubleVectorProperty* New();
  vtkTypeMacro(vtkSMDoubleVectorProperty, vtkSMPropertyTemplate);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns an unique identifier for the property's type.
   */
  int GetElementType() const override { return vtkSMProperty::DOUBLE; }

protected:
  vtkSMDoubleVectorProperty();
  ~vtkSMDoubleVectorProperty() override;

  double ToValue(const std::string& valueString) const override;
  vtkJson ToJson(const double& value) const override;
  double ToValue(const vtkJson& json, vtkSMStateContext* context) const override;

private:
  vtkSMDoubleVectorProperty(const vtkSMDoubleVectorProperty&) = delete;
  void operator=(const vtkSMDoubleVectorProperty&) = delete;
};

#endif
