/*=========================================================================

  Program:   ParaView
  Module:    vtkInformationProvider.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationProvider.h"

#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkObjectWrapper.h"
#include "vtkPVInformation.h"
#include "vtkSMInstantiator.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"

class vtkInformationProvider::vtkInternals
{
public:
  vtkInformationProvider* Self;
  vtkSmartPointer<vtkObjectStore> ObjectStore;
  vtkSmartPointer<vtkMultiProcessController> Controller;
  std::vector<rxcpp::composite_subscription> Subscriptions;

  vtkInternals(vtkInformationProvider* self)
    : Self(self)
  {
  }

  ~vtkInternals()
  {
    for (auto& subscription : this->Subscriptions)
    {
      subscription.unsubscribe();
    }
  }

  vtkJson GatherInformation(const vtkJson& request) const;
};

//-----------------------------------------------------------------------------
vtkJson vtkInformationProvider::vtkInternals::GatherInformation(const vtkJson& request) const
{
  const auto infoClassName = request.at("class").get<std::string>();
  auto object = vtkSMInstantiator::CreateInstance(infoClassName);
  auto* info = vtkPVInformation::SafeDownCast(object);

  // pass the service's controller for any distributed collection the
  // information object has to do.
  info->SetController(this->Controller);

  // initialize the information object.
  info->LoadState(request.at("state"));

  // gather information.
  auto target = this->ObjectStore->FindObject(request.at("target_gid").get<vtkTypeUInt32>());
  auto* targetWrapper = vtkObjectWrapper::SafeDownCast(target);
  info->GatherInformation(targetWrapper ? targetWrapper->GetVTKObject() : nullptr);

  return info->SaveInformation();
}

//============================================================================
vtkStandardNewMacro(vtkInformationProvider);
//----------------------------------------------------------------------------
vtkInformationProvider::vtkInformationProvider()
  : Internals(new vtkInformationProvider::vtkInternals(this))
{
}

//----------------------------------------------------------------------------
vtkInformationProvider::~vtkInformationProvider()
{
  vtkLogF(INFO, "vtkInformationProvider::~vtkInformationProvider()");
}

//----------------------------------------------------------------------------
void vtkInformationProvider::SetObjectStore(vtkObjectStore* store)
{
  auto& internals = (*this->Internals);
  internals.ObjectStore = store;
}

//----------------------------------------------------------------------------
vtkObjectStore* vtkInformationProvider::GetObjectStore() const
{
  const auto& internals = (*this->Internals);
  return internals.ObjectStore;
}

//----------------------------------------------------------------------------
void vtkInformationProvider::SetController(vtkMultiProcessController* controller)
{
  auto& internals = (*this->Internals);
  internals.Controller = controller;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkInformationProvider::GetController() const
{
  const auto& internals = (*this->Internals);
  return internals.Controller;
}

//----------------------------------------------------------------------------
vtkPacket vtkInformationProvider::GatherInformation(
  vtkPVInformation* information, vtkTypeUInt32 targetGID)
{
  vtkJson json = vtkJson::object();
  json["class"] = information->GetClassName();
  json["target_gid"] = targetGID;
  json["state"] = information->SaveState();
  return { json };
}

//----------------------------------------------------------------------------
void vtkInformationProvider::InitializeInternal(vtkService* service)
{
  assert(service != nullptr);
  auto& internals = (*this->Internals);
  auto subscription =
    service->GetRequestObservable(/*skipEventLoop*/ true)
      .filter([](const vtkServiceReceiver& receiver) {
        const auto& json = receiver.GetPacket().GetJSON();
        auto iter = json.find("type");
        return (iter != json.end() && iter.value().get<std::string>() == "vtk-information-collect");
      })
      .observe_on(service->GetRunLoopScheduler())
      .subscribe([&internals](const vtkServiceReceiver& receiver) {
        const auto& packet = receiver.GetPacket();
        const auto& json = packet.GetJSON();
        vtkLogF(INFO, "%s", json.dump(-1).c_str());
        receiver.Respond(internals.GatherInformation(json));
      });

  internals.Subscriptions.push_back(subscription);
}

//----------------------------------------------------------------------------
void vtkInformationProvider::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
