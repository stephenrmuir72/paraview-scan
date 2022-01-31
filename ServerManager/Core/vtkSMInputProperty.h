/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMInputProperty
 * @brief
 *
 */

#ifndef vtkSMInputProperty_h
#define vtkSMInputProperty_h

#include "vtkSMOutputPort.h" // for vtkSMOutputPort
#include "vtkSMPropertyTemplate.h"
#include "vtkServerManagerCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"            // for vtkSmartPointer

class VTKSERVERMANAGERCORE_EXPORT vtkSMInputProperty
  : public vtkSMPropertyTemplate<vtkSmartPointer<vtkSMOutputPort>>
{
public:
  static vtkSMInputProperty* New();
  vtkTypeMacro(vtkSMInputProperty, vtkSMPropertyTemplate);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetElementType() const override { return vtkSMProperty::INPUT; }

  /**
   * Overridden to add observers for inputs and update producer/consumer
   * relationships.
   */
  void Modified() override;

  vtkSMProxy* GetProxy(size_t index) const
  {
    auto port = this->GetElement(index);
    return port ? port->GetProducer() : nullptr;
  }

  vtkSMProxy* GetUncheckedProxy(size_t index) const
  {
    auto port = this->GetUncheckedElement(index);
    return port ? port->GetProducer() : nullptr;
  }

  int GetOutputPortForConnection(size_t index) const
  {
    auto port = this->GetElement(index);
    return port ? port->GetIndex() : 0;
  }

  int GetUncheckedOutputPortForConnection(size_t index) const
  {
    auto port = this->GetUncheckedElement(index);
    return port ? port->GetIndex() : 0;
  }

  bool RemoveProxy(vtkSMProxy* proxy);

protected:
  vtkSMInputProperty();
  ~vtkSMInputProperty();

  vtkSmartPointer<vtkSMOutputPort> ToValue(const std::string& valueString) const override;
  vtkJson ToJson(const vtkSmartPointer<vtkSMOutputPort>& value) const override;
  vtkSmartPointer<vtkSMOutputPort> ToValue(
    const vtkJson& json, vtkSMStateContext* context) const override;

private:
  vtkSMInputProperty(const vtkSMInputProperty&) = delete;
  void operator=(const vtkSMInputProperty&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  class vtkObserver;
  friend class vtkObserver;
  void InputDataInformationUpdated();
};

#endif
