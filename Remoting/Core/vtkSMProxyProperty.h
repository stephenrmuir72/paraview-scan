/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMProxyProperty
 * @brief
 *
 */

#ifndef vtkSMProxyProperty_h
#define vtkSMProxyProperty_h

#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkSMPropertyTemplate.h"
#include "vtkSMProxy.h"      // for vtkSMProxy
#include "vtkSmartPointer.h" // for vtkSmartPointer

class VTKREMOTINGCORE_EXPORT vtkSMProxyProperty
  : public vtkSMPropertyTemplate<vtkSmartPointer<vtkSMProxy>>
{
public:
  static vtkSMProxyProperty* New();
  vtkTypeMacro(vtkSMProxyProperty, vtkSMPropertyTemplate);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetElementType() const override { return vtkSMProperty::PROXY; }

protected:
  vtkSMProxyProperty();
  ~vtkSMProxyProperty() override;

  vtkSmartPointer<vtkSMProxy> ToValue(const std::string& valueString) const override;
  vtkJson ToJson(const vtkSmartPointer<vtkSMProxy>& value) const override;
  vtkSmartPointer<vtkSMProxy> ToValue(
    const vtkJson& json, vtkSMStateContext* context) const override;

private:
  vtkSMProxyProperty(const vtkSMProxyProperty&) = delete;
  void operator=(const vtkSMProxyProperty&) = delete;
};

#endif
