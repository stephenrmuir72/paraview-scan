/*=========================================================================

  Program:   ParaView
  Module:    vtkInteractionProvider.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractionProvider.h"

#include "vtkCamera.h"
#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkPVLogger.h"
#include "vtkPVView.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"
#include "vtkSynchronizedRenderers.h"

class vtkInteractionProvider::vtkInternals
{
public:
  vtkInteractionProvider* Self;
  vtkSmartPointer<vtkObjectStore> ObjectStore;
  rxcpp::composite_subscription Subscription;
  std::atomic<uint64_t> Tag{ 0 }; // TODO: move to the view

  vtkInternals(vtkInteractionProvider* self)
    : Self(self)
  {
  }

  ~vtkInternals() { this->Subscription.unsubscribe(); }

  void Render(vtkTypeUInt32 gid, vtkCamera* camera)
  {
    auto view = this->ObjectStore->FindVTKObject<vtkPVView>(gid);
    if (!view)
    {
      vtkLogF(ERROR, "Render called on non-view object!");
      return;
    }

    if (auto* activeCamera = view->GetActiveCamera())
    {
      view->GetActiveCamera()->DeepCopy(camera);
    }

    // TODO: or interactive render -- we'll need to add support to
    // enter/leave interactive rendering mode appropriately.
    vtkVLogStartScope(vtkPVLogger::PARAVIEW_LOG_RENDERING_VERBOSITY(), "still-render");
    view->StillRender();
    vtkLogEndScope("still-render");
  }

  static void FromJSON(vtkCamera* camera, const vtkNJson& json)
  {
    double viewUp[3];
    double position[3];
    double focalPoint[3];
    double viewAngle;

    VTK_NJSON_LOAD_MEMBER_ARRAY(json, viewUp);
    VTK_NJSON_LOAD_MEMBER_ARRAY(json, position);
    VTK_NJSON_LOAD_MEMBER_ARRAY(json, focalPoint);
    VTK_NJSON_LOAD_MEMBER(json, viewAngle);

    camera->SetViewUp(viewUp);
    camera->SetPosition(position);
    camera->SetFocalPoint(focalPoint);
    camera->SetViewAngle(viewAngle);
  }

  static vtkNJson ToJSON(vtkCamera* camera)
  {
    double viewUp[3];
    camera->GetViewUp(viewUp);

    double position[3];
    camera->GetPosition(position);

    double focalPoint[3];
    camera->GetFocalPoint(focalPoint);

    double viewAngle = camera->GetViewAngle();

    vtkNJson json;
    VTK_NJSON_SAVE_MEMBER_ARRAY(json, viewUp);
    VTK_NJSON_SAVE_MEMBER_ARRAY(json, position);
    VTK_NJSON_SAVE_MEMBER_ARRAY(json, focalPoint);
    VTK_NJSON_SAVE_MEMBER(json, viewAngle);
    return json;
  }
};

vtkStandardNewMacro(vtkInteractionProvider);
//----------------------------------------------------------------------------
vtkInteractionProvider::vtkInteractionProvider()
  : Internals(new vtkInteractionProvider::vtkInternals(this))
{
}

//----------------------------------------------------------------------------
vtkInteractionProvider::~vtkInteractionProvider() = default;

//----------------------------------------------------------------------------
void vtkInteractionProvider::SetObjectStore(vtkObjectStore* store)
{
  auto& internals = (*this->Internals);
  internals.ObjectStore = store;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkObjectStore> vtkInteractionProvider::GetObjectStore() const
{
  const auto& internals = (*this->Internals);
  return internals.ObjectStore;
}

//----------------------------------------------------------------------------
vtkPacket vtkInteractionProvider::Render(vtkTypeUInt32 gid, vtkCamera* camera)
{
  vtkNJson json = vtkNJson::object();
  json["type"] = "vtk-interaction-render";
  json["gid"] = gid;
  json["camera"] = vtkInternals::ToJSON(camera);
  return { json };
}

//----------------------------------------------------------------------------
void vtkInteractionProvider::InitializeInternal(vtkService* service)
{
  assert(service != nullptr);
  auto& internals = (*this->Internals);
  auto* sController = service->GetController();
  const int numRanks = sController->GetNumberOfProcesses();

  service->GetRequestObservable(/*skipEventLoop*/ true)
    .filter([](const vtkServiceReceiver& receiver) {
      const auto& json = receiver.GetPacket().GetJSON();
      auto iter = json.find("type");
      return (iter != json.end() && iter.value().get<std::string>() == "vtk-interaction-render");
    })
    .map([&internals](const vtkServiceReceiver& receiver) {
      const uint64_t tag = ++internals.Tag;
      // vtkLogF(TRACE, "enqueue %ld", tag);
      const auto& json = receiver.GetPacket().GetJSON();
      vtkTypeUInt32 gid = json.at("gid").get<vtkTypeUInt32>();

      auto camera = vtk::TakeSmartPointer(vtkCamera::New());
      vtkInternals::FromJSON(camera, json.at("camera"));
      return std::make_tuple(tag, gid, camera);
    })
    .observe_on(service->GetRunLoopScheduler())
    .map(rxcpp::util::apply_to([&internals, sController, numRanks](uint64_t tag, vtkTypeUInt32 gid,
                                 const vtkSmartPointer<vtkCamera>& camera) {
      /**
       * This function ensures that all ranks make the same decision on
       * whether to render this message i.e. if this message is most recent
       * message any of the ranks has received.
       */
      uint64_t ltag = internals.Tag;
      uint64_t gtag = ltag;
      if (numRanks > 1)
      {
        sController->AllReduce(&ltag, &gtag, 1, vtkCommunicator::MAX_OP);
      }
      return std::make_tuple(tag == gtag, gid, camera);
    }))
    .filter(rxcpp::util::apply_to(
      [&internals](bool doRender, vtkTypeUInt32 vtkNotUsed(gid),
        const vtkSmartPointer<vtkCamera>& vtkNotUsed(camera)) { return doRender; }))
    .map(rxcpp::util::apply_to(
      [&internals](bool, vtkTypeUInt32 gid, const vtkSmartPointer<vtkCamera>& camera) {
        return std::make_tuple(gid, camera);
      }))
    .subscribe(internals.Subscription,
      rxcpp::util::apply_to(
        [&internals](vtkTypeUInt32 gid, const vtkSmartPointer<vtkCamera>& camera) {
          vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "do-render gid=%u", gid);
          internals.Render(gid, camera);
        }));
}

//----------------------------------------------------------------------------
void vtkInteractionProvider::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
