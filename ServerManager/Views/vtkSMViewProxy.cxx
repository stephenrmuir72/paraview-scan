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

#include "vtkCamera.h"
#include "vtkChannelSubscription.h"
#include "vtkClientSession.h"
#include "vtkCommand.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInteractionProvider.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplicationOptions.h"
#include "vtkPVCoreApplication.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkServiceEndpoint.h"
#include "vtkSmartPointer.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkUnsignedCharArray.h"

class vtkSMViewProxy::vtkInternals
{
public:
  vtkSMViewProxy* Self{ nullptr };
  vtkSmartPointer<vtkRenderWindow> RenderWindow;
  vtkSmartPointer<vtkRenderer> Renderer;
  unsigned long RenderWindowObserverId{ 0 };

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
      vtkLogF(INFO, "PushToViewport");
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
      vtkLogF(INFO, "push camera");
      this->LastCameraMTime = camera->GetMTime();
      auto packet = vtkInteractionProvider::Render(this->Self->GetGlobalID(), camera);
      this->Self->GetSession()->SendMessage(vtkClientSession::RENDER_SERVER, std::move(packet));
    }
  }

  void UpdateSize()
  {
    auto& window = this->RenderWindow;
    vtkSMIntVectorProperty::SafeDownCast(this->Self->GetProperty("ViewSize"))
      ->SetElements({ window->GetSize()[0], window->GetSize()[1] });
    vtkLogF(INFO, "UpdateSize(%d,%d)", window->GetSize()[0], window->GetSize()[1]);
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
  this->RenderWindow->RemoveObserver(this->RenderWindowObserverId);
  this->RenderWindowObserverId = 0;
}

vtkStandardNewMacro(vtkSMViewProxy);
//----------------------------------------------------------------------------
vtkSMViewProxy::vtkSMViewProxy()
  : Internals(new vtkSMViewProxy::vtkInternals())
{
  auto& internals = (*this->Internals);
  internals.Self = this;

  auto* options = vtkPVCoreApplication::GetInstance()->GetOptions();
  if (vtkPVView::GetUseGenericOpenGLRenderWindow())
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
  internals.Renderer->SetBackground(0, 0, 0.4);
  internals.RenderWindow->AddRenderer(internals.Renderer);

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
void vtkSMViewProxy::PostRenderingResult(const vtkPacket& packet)
{
  auto& internals = (*this->Internals);
  vtkLogF(INFO, "got image");
  const auto& json = packet.GetJSON();
#if 0
  auto& binary = json.at("image").get_binary();

  vtkNew<vtkUnsignedCharArray> array;
  array->SetNumberOfComponents(4);
  array->SetNumberOfTuples(binary.size() / 4);
  std::copy(binary.begin(), binary.end(), array->GetPointer(0));
#else
  auto ptr = json.at("image").get<uintptr_t>();
  auto array = vtk::TakeSmartPointer(reinterpret_cast<vtkUnsignedCharArray*>(ptr));
#endif

  vtkSynchronizedRenderers::vtkRawImage image;
  image.Initialize(json.at("width").get<int>(), json.at("height").get<int>(), array);
  image.MarkValid();
  internals.PushImage(std::move(image));
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
