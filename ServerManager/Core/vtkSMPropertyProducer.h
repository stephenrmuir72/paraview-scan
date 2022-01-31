/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyProducer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMPropertyProducer_h
#define vtkSMPropertyProducer_h

#include "vtkSMProxy.h"     // for vtkSMProxy
#include "vtkWeakPointer.h" // for vtkWeakPointer

#include <set>    // for std::set
#include <vector> // for std::vector

/**
 * Internal class used vtkSMProxyProperty and vtkSMInputProperty to track
 * producers.
 */
class vtkSMPropertyProducer
{
public:
  vtkSMPropertyProducer() = default;
  ~vtkSMPropertyProducer() = default;

  void UpdateProducers(vtkSMProperty* self, const std::vector<vtkSMProxy*>& inputs)
  {
    std::set<vtkSMProxy*> new_producers;
    std::copy(inputs.begin(), inputs.end(), std::inserter(new_producers, new_producers.end()));

    std::set<vtkSMProxy*> old_producers;
    for (auto& producer : this->Producers)
    {
      if (producer)
      {
        old_producers.insert(producer);
      }
    }

    std::vector<vtkSMProxy*> to_add;
    std::set_difference(new_producers.begin(), new_producers.end(), old_producers.begin(),
      old_producers.end(), std::back_inserter(to_add));

    std::vector<vtkSMProxy*> to_remove;
    std::set_difference(old_producers.begin(), old_producers.end(), new_producers.begin(),
      new_producers.end(), std::back_inserter(to_remove));

    auto* parentProxy = self->GetParentProxy();
    for (auto& proxy : to_remove)
    {
      proxy->RemoveConsumer(parentProxy, self);
    }
    for (auto& proxy : to_add)
    {
      proxy->AddConsumer(parentProxy, self);
    }
    this->Producers.clear();
    std::copy(new_producers.begin(), new_producers.end(), std::back_inserter(this->Producers));
  }

private:
  vtkSMPropertyProducer(const vtkSMPropertyProducer&) = delete;
  void operator=(const vtkSMPropertyProducer&) = delete;

  std::vector<vtkWeakPointer<vtkSMProxy>> Producers;
};

#endif
