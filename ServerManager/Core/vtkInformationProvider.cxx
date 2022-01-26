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

#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkObjectWrapper.h"
#include "vtkPVInformation.h"
#include "vtkPVLogger.h"
#include "vtkReflection.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"

class vtkInformationProvider::vtkInternals
{
public:
  vtkInformationProvider* Self;
  vtkSmartPointer<vtkObjectStore> ObjectStore;
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
  auto info = vtkReflection::CreateInstance<vtkPVInformation>(infoClassName);

  auto* controller = this->Self->GetController();
  const auto rank = controller->GetLocalProcessId();
  const auto numRanks = controller->GetNumberOfProcesses();
  if (info->GetRootOnly() && rank > 0)
  {
    return {};
  }

  // initialize the information object.
  info->LoadState(request.at("state"));

  // gather information.
  auto target = this->ObjectStore->FindObject(request.at("target_gid").get<vtkTypeUInt32>());
  auto* targetWrapper = vtkObjectWrapper::SafeDownCast(target);
  info->GatherInformation(targetWrapper ? targetWrapper->GetVTKObject() : nullptr);
  auto state = info->SaveInformation();
  if (numRanks > 1)
  {
    vtkMultiProcessStream stream;
    if (rank > 0)
    {
      auto bson = vtkJson::to_bson(state);
      stream << static_cast<unsigned int>(bson.size());
      stream.Push(
        reinterpret_cast<unsigned char*>(&bson[0]), static_cast<unsigned int>(bson.size()));
    }

    std::vector<vtkMultiProcessStream> streams;
    controller->Gather(stream, streams, 0);
    if (rank == 0)
    {
      streams.erase(streams.begin());
      for (auto& mpStream : streams)
      {
        unsigned int size;
        mpStream >> size;

        std::vector<uint8_t> bson(size);
        unsigned char* ptr = reinterpret_cast<unsigned char*>(&bson[0]);
        mpStream.Pop(ptr, size);

        auto clone = vtk::TakeSmartPointer(info->NewInstance());
        clone->LoadState(vtkJson::from_bson(bson));
        info->AddInformation(clone);
      }
      state = info->SaveInformation();
    }
  }
  return state;
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
  vtkVLogF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "vtkInformationProvider::~vtkInformationProvider()");
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
vtkPacket vtkInformationProvider::GatherInformation(
  vtkPVInformation* information, vtkTypeUInt32 targetGID)
{
  vtkJson json = vtkJson::object();
  json["type"] = "vtk-information-collect";
  json["class"] = information->GetClassName();
  json["target_gid"] = targetGID;
  json["state"] = information->SaveState();
  return { json };
}

//----------------------------------------------------------------------------
bool vtkInformationProvider::ParseResponse(const vtkPacket& packet, vtkPVInformation* information)
{
  return information->LoadInformation(packet.GetJSON());
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
        const auto txt = json.dump(-1);
        vtkVLogScopeF(PARAVIEW_LOG_PROVIDER_VERBOSITY(), "%s", txt.c_str());
        receiver.Respond(internals.GatherInformation(json));
      });

  internals.Subscriptions.push_back(subscription);
}

//----------------------------------------------------------------------------
void vtkInformationProvider::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
