/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewProxy.h"

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkChannelSubscription.h"
#include "vtkClientSession.h"
#include "vtkCommand.h"
#include "vtkCornerAnnotation.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInteractionProvider.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplicationOptions.h"
#include "vtkPVCoreApplication.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkServiceEndpoint.h"
#include "vtkSmartPointer.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariant.h"

#include <vtk_fmt.h> // needed for `fmt`
// clang-format off
#include VTK_FMT(fmt/core.h)
// clang-format on

namespace vtkSMViewProxyNS
{
const char* GetRepresentationNameFromHints(const char* viewType, vtkPVXMLElement* hints, int port)
{
  if (!hints)
  {
    return nullptr;
  }

  for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; ++cc)
  {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (child == nullptr || child->GetName() == nullptr)
    {
      continue;
    }

    // LEGACY: support DefaultRepresentations hint.
    // <Hints>
    //    <DefaultRepresentations representation="Foo" />
    // </Hints>
    if (strcmp(child->GetName(), "DefaultRepresentations") == 0)
    {
      return child->GetAttribute("representation");
    }

    // <Hints>
    //    <Representation port="outputPort" view="ViewName" type="ReprName" />
    // </Hints>
    else if (strcmp(child->GetName(), "Representation") == 0 &&
      // has an attribute "view" that matches the viewType.
      child->GetAttribute("view") && strcmp(child->GetAttribute("view"), viewType) == 0 &&
      child->GetAttribute("type") != nullptr)
    {
      // if port is present, it must match "port".
      int xmlPort;
      if (child->GetScalarAttribute("port", &xmlPort) == 0 || xmlPort == port)
      {
        return child->GetAttribute("type");
      }
    }
  }
  return nullptr;
}

} // namespace vtkSMViewProxyNS

class vtkSMViewProxy::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkSMViewProxy* Self{ nullptr };
  vtkSmartPointer<vtkRenderWindow> RenderWindow;
  vtkSmartPointer<vtkRenderer> Renderer;
  unsigned long RenderWindowObserverId{ 0 };
  std::string DefaultRepresentationName;
  vtkBoundingBox VisiblePropBounds;
  vtkSmartPointer<vtkCornerAnnotation> CornerAnnotation;
  vtkSmartPointer<vtkRenderer> Renderer2D;

  void SetupInteractor();

  ~vtkInternals()
  {
    if (auto* iren = this->RenderWindow->GetInteractor())
    {
      for (const auto& id : this->InteractorObserverIds)
      {
        iren->RemoveObserver(id);
      }
    }

    if (this->RenderWindowObserverId)
    {
      this->RenderWindow->RemoveObserver(this->RenderWindowObserverId);
    }
  }

  void PushImageToWindow()
  {
    if (this->Image.IsValid())
    {
      vtkLogF(TRACE, "PushToViewport");
      this->Image.PushToViewport(this->Renderer);
    }
  }

  void PushImage(vtkSynchronizedRenderers::vtkRawImage&& image)
  {
    this->InRender = true;
    this->Image = image;
    this->RenderWindow->Render();
    this->InRender = false;
  }

  void RequestRender() { this->UpdateCameraIfNeeded(/*force*/ true); }

  void AddMetadataAsAnnotations(vtkNJson metadata)
  {
    if (metadata.empty())
    {
      return;
    }
    if (metadata.contains("rendering-pass") && metadata.contains("total-rendering-passes"))
    {
      int pass = metadata.at("rendering-pass").get<int>();
      int totalPasses = metadata.at("total-rendering-passes").get<int>();
      std::string text = fmt::format("Rendering Pass {:3d}/{:3d}", pass, totalPasses);
      this->CornerAnnotation->SetText(2, text.c_str());
      this->CornerAnnotation->SetVisibility(true);
    }
  }

private:
  std::vector<unsigned long> InteractorObserverIds;
  vtkMTimeType LastCameraMTime{ 0 };
  bool InRender{ false };
  vtkSynchronizedRenderers::vtkRawImage Image;

  void HandleInteractorEvent(vtkObject* /*caller*/, unsigned long eventId, void* /*calldata*/)
  {
    switch (eventId)
    {
      case vtkCommand::RenderEvent:
        if (!this->InRender)
        {
          this->UpdateCameraIfNeeded();
        }
        break;

      case vtkCommand::WindowResizeEvent:
      case vtkCommand::ConfigureEvent:
        this->UpdateSize();
        break;
    }
  }

  void UpdateCameraIfNeeded(bool force = false)
  {
    auto* camera = this->Renderer->GetActiveCamera();
    if (force || this->LastCameraMTime != camera->GetMTime())
    {
      vtkLogF(TRACE, "push camera");
      this->LastCameraMTime = camera->GetMTime();
      auto packet = vtkInteractionProvider::Render(this->Self->GetGlobalID(), camera);
      this->Self->GetSession()->SendMessage(vtkClientSession::RENDER_SERVER, std::move(packet));
    }
  }

  void UpdateSize()
  {
    auto& window = this->RenderWindow;
    vtkSMPropertyHelper(this->Self, "ViewSize").Set(window->GetSize(), 2);
    vtkLogF(TRACE, "UpdateSize(%d,%d)", window->GetSize()[0], window->GetSize()[1]);
    this->Self->UpdateVTKObjects();
    this->UpdateCameraIfNeeded(true);
  }
};

//----------------------------------------------------------------------------
void vtkSMViewProxy::vtkInternals::SetupInteractor()
{
  auto* iren = this->RenderWindow->GetInteractor();
  if (!iren)
  {
    return;
  }

  this->Self->InitializeInteractor(iren);

  this->InteractorObserverIds.push_back(iren->AddObserver(
    vtkCommand::StartInteractionEvent, this, &vtkInternals::HandleInteractorEvent));
  this->InteractorObserverIds.push_back(
    iren->AddObserver(vtkCommand::EndInteractionEvent, this, &vtkInternals::HandleInteractorEvent));
  this->InteractorObserverIds.push_back(
    iren->AddObserver(vtkCommand::RenderEvent, this, &vtkInternals::HandleInteractorEvent));
  this->InteractorObserverIds.push_back(
    iren->AddObserver(vtkCommand::WindowResizeEvent, this, &vtkInternals::HandleInteractorEvent));

  // XOpenGLRenderWindow fires ConfigureEvent when window resizes.
  this->InteractorObserverIds.push_back(
    iren->AddObserver(vtkCommand::ConfigureEvent, this, &vtkInternals::HandleInteractorEvent));

  // Remove the observer so we don't end up calling this method again.
  this->RenderWindow->RemoveObserver(this->RenderWindowObserverId);
  this->RenderWindowObserverId = 0;
}

//============================================================================
vtkStandardNewMacro(vtkSMViewProxy);
bool vtkSMViewProxy::UseGenericOpenGLRenderWindow = false;
//----------------------------------------------------------------------------
vtkSMViewProxy::vtkSMViewProxy()
  : Internals(new vtkSMViewProxy::vtkInternals())
{
  auto& internals = (*this->Internals);
  internals.Self = this;

  auto* options = vtkPVCoreApplication::GetInstance()->GetOptions();
  if (vtkSMViewProxy::GetUseGenericOpenGLRenderWindow())
  {
    internals.RenderWindow = vtk::TakeSmartPointer(vtkGenericOpenGLRenderWindow::New());
  }
  else if (options->GetForceOffscreenRendering())
  {
    internals.RenderWindow = vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();
  }
  else
  {
    internals.RenderWindow = vtk::TakeSmartPointer(vtkRenderWindow::New());
  }

  internals.Renderer = vtk::TakeSmartPointer(vtkRenderer::New());
  internals.Renderer->SetBackground(0, 0, 0); // must be black.
  internals.RenderWindow->AddRenderer(internals.Renderer);
  internals.RenderWindow->SetNumberOfLayers(3);

  // Setup Corner annotation for rendering passes
  internals.CornerAnnotation = vtk::TakeSmartPointer(vtkCornerAnnotation::New());
  internals.CornerAnnotation->SetLinearFontScaleFactor(2);
  internals.CornerAnnotation->SetNonlinearFontScaleFactor(1);
  internals.CornerAnnotation->SetMaximumFontSize(40);
  internals.CornerAnnotation->SetText(2, "");
  internals.CornerAnnotation->SetVisibility(true);

  internals.Renderer2D = vtk::TakeSmartPointer(vtkRenderer::New());
  internals.Renderer2D->SetLayer(2);
  internals.Renderer2D->EraseOff();
  internals.Renderer2D->InteractiveOff();
  internals.Renderer2D->SetActiveCamera(internals.Renderer->GetActiveCamera());
  internals.Renderer2D->AddViewProp(internals.CornerAnnotation);
  internals.RenderWindow->AddRenderer(internals.Renderer2D);

  // monitor render window so we can setup observers on interactor
  // for events that send render requests to the server.
  internals.RenderWindowObserverId = internals.RenderWindow->AddObserver(
    vtkCommand::ModifiedEvent, &internals, &vtkSMViewProxy::vtkInternals::SetupInteractor);
  internals.Renderer->AddObserver(
    vtkCommand::EndEvent, &internals, &vtkSMViewProxy::vtkInternals::PushImageToWindow);
}

//----------------------------------------------------------------------------
vtkSMViewProxy::~vtkSMViewProxy() = default;

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMViewProxy::GetRenderWindow() const
{
  const auto& internals = (*this->Internals);
  return internals.RenderWindow;
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkSMViewProxy::GetRenderWindowInteractor() const
{
  const auto& internals = (*this->Internals);
  return internals.RenderWindow ? internals.RenderWindow->GetInteractor() : nullptr;
}

//----------------------------------------------------------------------------
vtkCamera* vtkSMViewProxy::GetCamera() const
{
  const auto& internals = (*this->Internals);
  return internals.Renderer->GetActiveCamera();
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::InitializeInteractor(vtkRenderWindowInteractor* vtkNotUsed(iren)) {}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PostRenderingResult(const vtkPacket& packet)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  vtkLogF(TRACE, "got image");
  const auto& json = packet.GetJSON();
  auto array = packet.GetPayload<vtkUnsignedCharArray>("image");
  assert(array);
  vtkSynchronizedRenderers::vtkRawImage image;
  image.Initialize(json.at("width").get<int>(), json.at("height").get<int>(), array);
  image.MarkValid();

  if (json.contains("metadata"))
  {
    internals.AddMetadataAsAnnotations(json["metadata"]);
  }
  else
  {
    internals.CornerAnnotation->SetVisibility(false);
  }

  internals.PushImage(std::move(image));
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::SetUseGenericOpenGLRenderWindow(bool val)
{
  vtkSMViewProxy::UseGenericOpenGLRenderWindow = val;
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::GetUseGenericOpenGLRenderWindow()
{
  return vtkSMViewProxy::UseGenericOpenGLRenderWindow;
}

//----------------------------------------------------------------------------
rxcpp::observable<bool> vtkSMViewProxy::Update()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  using BehaviorT = rxcpp::subjects::behavior<vtkVariant>;
  auto behavior = std::make_shared<BehaviorT>(vtkVariant());
  this->ExecuteCommand("Update", vtkClientSession::DATA_SERVER).subscribe([behavior, this](bool) {
    this->ExecuteCommand("UpdateForRendering", vtkClientSession::RENDER_SERVER)
      .subscribe([=](bool status) {
        behavior->get_subscriber().on_next(vtkVariant(status ? 1 : 0));
        this->StillRender();
      });
  });

  return behavior->get_observable()
    .filter([](const vtkVariant& variant) { return variant.IsValid(); })
    .take(1)
    .map([behavior](const vtkVariant& variant) {
      // capture behavior to ensure it hangs around until the returned value hangs around.
      (void)behavior;
      return variant.ToInt() != 0;
    });
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::ResetCameraUsingVisiblePropBounds()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (internals.VisiblePropBounds.IsValid())
  {
    double bds[6];
    internals.VisiblePropBounds.GetBounds(bds);
    internals.Renderer->ResetCamera(bds);

    // If 'CenterOfRotation' is present, set it to the center of bounds as well.
    if (auto* centerProperty = this->GetProperty("CenterOfRotation"))
    {
      double center[3];
      internals.VisiblePropBounds.GetCenter(center);
      vtkSMPropertyHelper(centerProperty).Set(center, 3);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::StillRender()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.RequestRender();
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::HideOtherRepresentationsIfNeeded(vtkSMProxy* repr)
{
  if (repr == nullptr || this->GetHints() == nullptr ||
    this->GetHints()->FindNestedElementByName("ShowOneRepresentationAtATime") == nullptr)
  {
    return false;
  }

  vtkPVXMLElement* oneRepr =
    this->GetHints()->FindNestedElementByName("ShowOneRepresentationAtATime");
  const char* reprType = oneRepr->GetAttribute("type");

  if (reprType && strcmp(repr->GetXMLName(), reprType) != 0)
  {
    return false;
  }

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  bool modified = false;
  vtkSMPropertyHelper helper(this, "Representations");
  for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
  {
    auto* arepr = helper.GetAsProxy(cc);
    if (arepr && arepr != repr)
    {
      if (vtkSMPropertyHelper(arepr, "Visibility", /*quiet*/ true).GetAsInt() == 1 &&
        (!reprType || (reprType && !strcmp(arepr->GetXMLName(), reprType))))
      {
        controller->Hide(arepr, this);
        modified = true;
      }
    }
  }
  return modified;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RepresentationVisibilityChanged(vtkSMProxy*, bool) {}

//----------------------------------------------------------------------------
int vtkSMViewProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
  {
    return 0;
  }

  auto& internals = (*this->Internals);
  const char* repr_name = element->GetAttribute("representation_name");
  if (repr_name)
  {
    internals.DefaultRepresentationName = repr_name;
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMViewProxy::CreateDefaultRepresentation(vtkSMProxy* proxy, int outputPort)
{
  vtkSMSourceProxy* producer = vtkSMSourceProxy::SafeDownCast(proxy);
  if ((producer == nullptr) || (outputPort < 0) ||
    (static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort) ||
    (producer->GetSession() != this->GetSession()))
  {
    return nullptr;
  }

  // Update with time from the view to ensure we have up-to-date data.
  double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
  producer->UpdatePipeline(view_time);

  const char* representationType = this->GetRepresentationType(producer, outputPort);
  if (!representationType)
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  if (auto* repr = pxm->NewProxy("representations", representationType))
  {
    return repr;
  }
  vtkWarningMacro(
    "Failed to create representation (representations," << representationType << ").");
  return nullptr;
}

//----------------------------------------------------------------------------
const char* vtkSMViewProxy::GetRepresentationType(vtkSMSourceProxy* producer, int outputPort)
{
  assert(producer && static_cast<int>(producer->GetNumberOfOutputPorts()) > outputPort);

  // Process producer hints to see if indicates what type of representation
  // to create for this view.
  if (const char* reprName = vtkSMViewProxyNS::GetRepresentationNameFromHints(
        this->GetXMLName(), producer->GetHints(), outputPort))
  {
    return reprName;
  }

  const auto& internals = (*this->Internals);
  // check if we have default representation name specified in XML.
  if (!internals.DefaultRepresentationName.empty())
  {
    vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
    vtkSMProxy* prototype =
      pxm->GetPrototypeProxy("representations", internals.DefaultRepresentationName.c_str());
    if (prototype)
    {
      vtkSMProperty* inputProp = prototype->GetProperty("Input");
      vtkSMUncheckedPropertyHelper helper(inputProp);
      helper.Set(producer, outputPort);
      bool acceptable = (inputProp->IsInDomains() > 0);
      helper.SetNumberOfElements(0);

      if (acceptable)
      {
        return internals.DefaultRepresentationName.c_str();
      }
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::CanDisplayData(vtkSMSourceProxy* producer, int outputPort)
{
  if (producer == nullptr || outputPort < 0 ||
    static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort ||
    producer->GetSession() != this->GetSession())
  {
    return false;
  }

  const char* type = this->GetRepresentationType(producer, outputPort);
  if (type != nullptr)
  {
    vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
    return (pxm->GetPrototypeProxy("representations", type) != nullptr);
  }

  return false;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMViewProxy::FindRepresentation(vtkSMSourceProxy* producer, int outputPort)
{
  vtkSMPropertyHelper helper(this, "Representations");
  for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
  {
    auto* repr = helper.GetAsProxy(cc);
    if (repr && repr->GetProperty("Input"))
    {
      vtkSMPropertyHelper helper2(repr, "Input");
      if (helper2.GetAsProxy() == producer &&
        static_cast<int>(helper2.GetOutputPort()) == outputPort)
      {
        return repr;
      }
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMViewProxy::FindView(vtkSMProxy* repr, const char* reggroup /*=views*/)
{
  if (!repr)
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = repr->GetSessionProxyManager();
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(pxm);
  iter->SetModeToOneGroup();
  for (iter->Begin(reggroup); !iter->IsAtEnd(); iter->Next())
  {
    if (vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(iter->GetProxy()))
    {
      auto reprs = vtkSMProxyProperty::SafeDownCast(view->GetProperty("Representations"));
      auto hreprs = vtkSMProxyProperty::SafeDownCast(view->GetProperty("HiddenRepresentations"));
      if ((reprs != nullptr && reprs->IsProxyAdded(repr)) ||
        (hreprs != nullptr && hreprs->IsProxyAdded(repr)))
      {
        return view;
      }
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::ProcessPiggybackedInformation(const vtkNJson& json)
{
  this->Superclass::ProcessPiggybackedInformation(json);

  auto iter = json.find("VisiblePropBounds");
  if (iter != json.end())
  {
    std::vector<double> bounds = iter.value().get<std::vector<double>>();
    this->Internals->VisiblePropBounds.SetBounds(&bounds[0]);
  }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
