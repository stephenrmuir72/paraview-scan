/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputProperty.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyProducer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkWeakPointer.h"

namespace
{
class vtkObserverHelper
{
  vtkWeakPointer<vtkSMOutputPort> Port;
  unsigned long ObserverId;

public:
  vtkObserverHelper(vtkSMOutputPort* port, vtkCommand* observer)
  {
    this->ObserverId = port ? port->AddObserver(vtkCommand::UpdateInformationEvent, observer) : 0;
  }
  ~vtkObserverHelper()
  {
    if (this->Port && this->ObserverId != 0)
    {
      this->Port->RemoveObserver(this->ObserverId);
    }
  }
};

} // end of namespace

class vtkSMInputProperty::vtkObserver : public vtkCommand
{
public:
  vtkTypeMacro(vtkObserver, vtkCommand);
  static vtkSMInputProperty::vtkObserver* New() { return new vtkSMInputProperty::vtkObserver; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventid),
    void* vtkNotUsed(calldata)) override
  {
    if (this->Property)
    {
      this->Property->InputDataInformationUpdated();
    }
  }
  vtkSMInputProperty* Property{ nullptr };
};

class vtkSMInputProperty::vtkInternals
{
  vtkNew<vtkSMInputProperty::vtkObserver> Observer;
  std::vector<vtkObserverHelper> Helpers;

  vtkSMPropertyProducer ProducerHelper;

public:
  vtkInternals(vtkSMInputProperty* property) { this->Observer->Property = property; }
  ~vtkInternals() { this->Observer->Property = nullptr; }

  void UpdateObservers(const std::vector<vtkSmartPointer<vtkSMOutputPort>>& inputs)
  {
    std::vector<vtkObserverHelper> helpers;
    std::transform(inputs.begin(), inputs.end(), std::back_inserter(helpers),
      [this](vtkSMOutputPort* port) { return vtkObserverHelper(port, this->Observer); });
    this->Helpers = std::move(helpers);
  }

  void UpdateProducers(
    vtkSMProperty* self, const std::vector<vtkSmartPointer<vtkSMOutputPort>>& inputs)
  {
    std::vector<vtkSMProxy*> proxies;
    std::transform(inputs.begin(), inputs.end(), std::back_inserter(proxies),
      [](auto& port) { return port->GetProducer(); });
    this->ProducerHelper.UpdateProducers(self, proxies);
  }
};

vtkStandardNewMacro(vtkSMInputProperty);
//----------------------------------------------------------------------------
vtkSMInputProperty::vtkSMInputProperty()
  : Internals(new vtkSMInputProperty::vtkInternals(this))
{
}

//----------------------------------------------------------------------------
vtkSMInputProperty::~vtkSMInputProperty() = default;

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSMOutputPort> vtkSMInputProperty::ToValue(const std::string& valueString) const
{
  return nullptr;
}

//----------------------------------------------------------------------------
vtkJson vtkSMInputProperty::ToJson(const vtkSmartPointer<vtkSMOutputPort>& value) const
{
  vtkJson json = vtkJson::object();
  json["producer"] = value ? value->GetProducer()->GetGlobalID() : 0;
  json["index"] = value ? value->GetIndex() : 0;
  return json;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSMOutputPort> vtkSMInputProperty::ToValue(
  const vtkJson& json, vtkSMStateContext*) const
{
  auto* pxm = this->GetProxyManager();
  auto producer = pxm ? pxm->FindProxy(json.at("producer").get<vtkTypeUInt32>()) : nullptr;
  auto index = producer ? json.at("index").get<int>() : 0;
  return producer ? producer->GetOutputPort(index) : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMInputProperty::Modified()
{
  auto& internals = (*this->Internals);
  internals.UpdateObservers(this->GetElements());
  internals.UpdateProducers(this, this->GetElements());
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
void vtkSMInputProperty::InputDataInformationUpdated()
{
  this->UpdateDependentDomains();
}

//----------------------------------------------------------------------------
bool vtkSMInputProperty::RemoveProxy(vtkSMProxy* proxy)
{
  auto elements = this->GetElements();
  auto iter = std::remove_if(elements.begin(), elements.end(),
    [&](const auto& port) { return port != nullptr && port->GetProducer() == proxy; });
  elements.erase(iter, elements.end());
  return this->SetElements(elements);
}

//----------------------------------------------------------------------------
void vtkSMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
