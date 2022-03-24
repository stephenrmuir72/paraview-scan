/*=========================================================================

  Program:   ParaView
  Module:    vtkPVView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVView.h"

#include "vtkDataObject.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkPVCoreApplication.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVLogger.h"
#include "vtkPVProcessWindow.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkTimerLog.h"
#include "vtkViewLayout.h"

#include <cassert>
#include <map>
#include <sstream>

namespace
{
class vtkOffscreenOpenGLRenderWindow : public vtkGenericOpenGLRenderWindow
{
public:
  static vtkOffscreenOpenGLRenderWindow* New();
  vtkTypeMacro(vtkOffscreenOpenGLRenderWindow, vtkGenericOpenGLRenderWindow);

  void SetContext(vtkOpenGLRenderWindow* cntxt)
  {
    if (cntxt == this->Context)
    {
      return;
    }

    if (this->Context)
    {
      this->MakeCurrent();
      this->Finalize();
    }

    this->SetReadyForRendering(false);
    this->Context = cntxt;
    if (cntxt)
    {
      this->SetForceMaximumHardwareLineWidth(1);
      this->SetReadyForRendering(true);
      this->SetOwnContext(0);
    }
  }
  void MakeCurrent() override
  {
    if (this->Context)
    {
      this->Context->MakeCurrent();
      this->Superclass::MakeCurrent();
    }
  }

  bool IsCurrent() override { return this->Context ? this->Context->IsCurrent() : false; }

  void SetUseOffScreenBuffers(bool) override {}
  void SetShowWindow(bool) override {}

  void Render() override
  {
    if (this->Context && this->GetReadyForRendering())
    {
      this->InvokeEvent(vtkViewLayout::RequestUpdateLayoutEvent);
      if (!this->Initialized)
      {
        // ensures that context is created.
        vtkPVProcessWindow::PrepareForRendering();
        this->Context->MakeCurrent();
        this->GetState()->PushFramebufferBindings();
        this->OpenGLInit();
      }
      else
      {
        this->GetState()->PushFramebufferBindings();
      }
      this->Superclass::Render();
      if (auto state = this->GetState())
      {
        state->PopFramebufferBindings();
      }
      this->InvokeEvent(vtkViewLayout::RequestUpdateDisplayEvent);
    }
  }

  vtkOpenGLState* GetState() override
  {
    return this->Context ? this->Context->GetState() : this->Superclass::GetState();
  }

protected:
  vtkOffscreenOpenGLRenderWindow()
  {
    this->SetReadyForRendering(false);
    this->Superclass::SetUseOffScreenBuffers(true);
    this->Superclass::SetShowWindow(false);
  }
  ~vtkOffscreenOpenGLRenderWindow() override
  {
    // have to finalize here while GetState() will use this classes
    // vtable. In parent destuctors GetState will return a different
    // value causing resource/state issues.
    this->Finalize();
  }

private:
  vtkOffscreenOpenGLRenderWindow(const vtkOffscreenOpenGLRenderWindow&) = delete;
  void operator=(const vtkOffscreenOpenGLRenderWindow&) = delete;

  vtkSmartPointer<vtkOpenGLRenderWindow> Context;
};

vtkStandardNewMacro(vtkOffscreenOpenGLRenderWindow);
}

//============================================================================
class vtkPVView::vtkInternals
{
public:
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkTypeUInt32 GlobalID{ 0 };
  vtkVector2i Position{ 0, 0 };
  vtkVector2i Size{ 300, 300 };
  int PPI{ 72 };
  double ViewTime{ 0.0 };
  std::string LogName;
  bool Initialized{ false };
  bool InitializedForDataProcessing{ false };

  vtkSmartPointer<vtkPVDataDeliveryManager> DeliveryManager;
  std::vector<vtkSmartPointer<vtkPVDataRepresentation>> Representations;

  mutable vtkTimeStamp UpdateTimeStamp;

  /**
   * Keeps track of the active rendering pass.
   */
  mutable vtkInformation* ActiveRequest{ nullptr };

  mutable int LastRenderingPass{ 0 };
  mutable int LastRenderingMode{ vtkPVView::STILL_RENDER };
};

vtkInformationKeyMacro(vtkPVView, REQUEST_UPDATE_DATA, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_UPDATE_DATA_LOD, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_UPDATE_RENDERING_INFORMATION, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_PREPARE_FOR_RENDER, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_PREPARE_FOR_RENDER_LOD, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_RENDER, Request);
vtkInformationKeyMacro(vtkPVView, UPDATE_TIME_STEP, Double);
//============================================================================
bool vtkPVView::UseGenericOpenGLRenderWindow = false;
//----------------------------------------------------------------------------
void vtkPVView::SetUseGenericOpenGLRenderWindow(bool val)
{
  vtkPVView::UseGenericOpenGLRenderWindow = val;
}

//----------------------------------------------------------------------------
bool vtkPVView::GetUseGenericOpenGLRenderWindow()
{
  return vtkPVView::UseGenericOpenGLRenderWindow;
}

//============================================================================
vtkPVView::vtkPVView()
  : Internals(new vtkPVView::vtkInternals())
{
  auto& internals = (*this->Internals);
  internals.DeliveryManager.TakeReference(vtkPVDataDeliveryManager::New());
  internals.DeliveryManager->SetView(this);
}

//----------------------------------------------------------------------------
vtkPVView::~vtkPVView() = default;

//----------------------------------------------------------------------------
void vtkPVView::SetGlobalID(vtkTypeUInt32 gid)
{
  auto& internals = (*this->Internals);
  internals.GlobalID = gid;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVView::GetGlobalID() const
{
  const auto& internals = (*this->Internals);
  return internals.GlobalID;
}

//----------------------------------------------------------------------------
void vtkPVView::SetController(vtkMultiProcessController* controller)
{
  auto& internals = (*this->Internals);
  internals.Controller = controller;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVView::GetController() const
{
  const auto& internals = (*this->Internals);
  return internals.Controller;
}

//----------------------------------------------------------------------------
void vtkPVView::SetPosition(int xpos, int ypos)
{
  auto& internals = (*this->Internals);
  internals.Position = vtkVector2i(xpos, ypos);
}

//----------------------------------------------------------------------------
const int* vtkPVView::GetPosition() const
{
  const auto& internals = (*this->Internals);
  return internals.Position.GetData();
}

//----------------------------------------------------------------------------
void vtkPVView::SetSize(int width, int height)
{
  auto& internals = (*this->Internals);
  if (this->InTileDisplayMode() || this->InCaveDisplayMode())
  {
    // the size request is ignored.
  }
  else if (auto* window = this->GetRenderWindow())
  {
    const auto cur_size = window->GetActualSize();
    if (cur_size[0] != width || cur_size[1] != height)
    {
      window->SetSize(width, height);
    }
  }
  internals.Size = vtkVector2i(width, height);
}

//----------------------------------------------------------------------------
const int* vtkPVView::GetSize() const
{
  const auto& internals = (*this->Internals);
  return internals.Size.GetData();
}

//----------------------------------------------------------------------------
void vtkPVView::SetPPI(int ppi)
{
  auto& internals = (*this->Internals);
  internals.PPI = ppi;
  if (auto window = this->GetRenderWindow())
  {
    window->SetDPI(ppi);
  }
}

//----------------------------------------------------------------------------
int vtkPVView::GetPPI() const
{
  const auto& internals = (*this->Internals);
  return internals.PPI;
}

//----------------------------------------------------------------------------
void vtkPVView::SetViewTime(double value)
{
  auto& internals = (*this->Internals);
  internals.ViewTime = value;
}

//----------------------------------------------------------------------------
double vtkPVView::GetViewTime() const
{
  const auto& internals = (*this->Internals);
  return internals.ViewTime;
}

//----------------------------------------------------------------------------
void vtkPVView::SetLogName(const std::string& name)
{
  auto& internals = (*this->Internals);
  internals.LogName = name;
}

//----------------------------------------------------------------------------
const std::string& vtkPVView::GetLogName() const
{
  const auto& internals = (*this->Internals);
  return internals.LogName;
}

//----------------------------------------------------------------------------
void vtkPVView::InitializeForDataProcessing()
{
  auto& internals = (*this->Internals);
  assert("Not already initialized" && !internals.Initialized);
  internals.Initialized = true;
  internals.InitializedForDataProcessing = true;
}

//----------------------------------------------------------------------------
void vtkPVView::InitializeForRendering()
{
  auto& internals = (*this->Internals);
  assert("Not already initialized" && !internals.Initialized);
  internals.Initialized = true;
  internals.InitializedForDataProcessing = false;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVView::NewRenderWindow()
{
  auto& internals = (*this->Internals);
  assert("View has been initialized." && internals.Initialized);
  assert("Called on rendering service." && !internals.InitializedForDataProcessing);

  auto* pvapp = vtkPVCoreApplication::GetInstance();
  // FIXME:
  // auto config = vtkRemotingCoreConfiguration::GetInstance();
  // // A little bit of a hack. I am not what's a good place to setup the display
  // // environment for this rank. So doing it here.
  // config->HandleDisplayEnvironment();

  vtkSmartPointer<vtkRenderWindow> window;
  // FIXME: ASYNC
  // For now, just create offscreen window always...we need to rethink this
  // quite a bit.
  window = vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();
  if (window)
  {
    vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(),
      "created a `%s` as a new render window for view %s", vtkLogIdentifier(window),
      vtkLogIdentifier(this));

    window->AlphaBitPlanesOn();

    std::ostringstream str;
    switch (pvapp->GetApplicationType())
    {
      case vtkPVCoreApplication::SERVER:
        str << "ParaView Server";
        break;

      default:
        str << "ParaView";
        break;
    }
    if (pvapp->GetNumberOfRanks() > 1)
    {
      str << pvapp->GetRank();
    }
    window->SetWindowName(str.str().c_str());
    window->Register(this);
    return window;
  }

  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(),
    "created `nullptr` as a new render window for view %s", vtkLogIdentifier(this));
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVView::InTileDisplayMode()
{
  // auto serverInfo = this->Session->GetServerInformation();
  // return serverInfo->GetIsInTileDisplay();
  // FIXME: ASYNC
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVView::InCaveDisplayMode()
{
  // auto serverInfo = this->Session->GetServerInformation();
  // return serverInfo->GetIsInCave();
  // FIXME: ASYNC
  return false;
}

//----------------------------------------------------------------------------
void vtkPVView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVView::Update()
{
  vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: update view", this->GetLogName().c_str());
  this->InvokeEvent(vtkCommand::StartEvent);

  const auto& internals = (*this->Internals);
  if (!internals.InitializedForDataProcessing)
  {
    vtkLogF(WARNING, "Update called on non-data processing service. Ignored.");
    return;
  }

  vtkNew<vtkInformation> request;
  request->Set(vtkPVView::REQUEST_UPDATE_DATA());
  request->Set(vtkPVView::UPDATE_TIME_STEP(), internals.ViewTime);
  this->CallProcessViewRequest(request);
  internals.UpdateTimeStamp.Modified();

  // TODO:
  // reduce visible data size across all ranks.
  // this will then be used for other decision making.
  // This decision making will include which service to render on, perhaps i.e. render-server
  // or render-server-lite.
  this->InvokeEvent(vtkCommand::UpdateEvent);
  this->InvokeEvent(vtkCommand::EndEvent);
}

//----------------------------------------------------------------------------
void vtkPVView::UpdateForRendering()
{
  vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: update view", this->GetLogName().c_str());
  this->InvokeEvent(vtkCommand::StartEvent);
  const auto& internals = (*this->Internals);
  if (internals.InitializedForDataProcessing)
  {
    vtkLogF(WARNING, "UpdateForRendering called on data processing service. Ignored.");
    return;
  }

  vtkNew<vtkInformation> request;
  request->Set(vtkPVView::REQUEST_UPDATE_RENDERING_INFORMATION());
  this->CallProcessViewRequest(request);
  request->Remove(vtkPVView::REQUEST_UPDATE_RENDERING_INFORMATION());
  request->Clear();

  request->Set(vtkPVView::REQUEST_PREPARE_FOR_RENDER());
  this->CallProcessViewRequest(request);

  this->InvokeEvent(vtkCommand::UpdateEvent);
  this->InvokeEvent(vtkCommand::EndEvent);
}

//----------------------------------------------------------------------------
void vtkPVView::StillRender(int pass /*=0*/)
{
  vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: still-render", this->GetLogName().c_str());
  const auto& internals = (*this->Internals);
  if (internals.InitializedForDataProcessing)
  {
    vtkLogF(WARNING, "StillRender called on data processing service. Ignored.");
    return;
  }

  internals.LastRenderingPass = pass;
  internals.LastRenderingMode = vtkPVView::STILL_RENDER;

  vtkNew<vtkInformation> request;
  request->Set(vtkPVView::REQUEST_RENDER());
  this->CallProcessViewRequest(request);

  this->PrepareForRendering();
  if (auto* window = this->GetRenderWindow())
  {
    window->Render();
  }

  int interactiveRender = 0;
  this->InvokeEvent(vtkCommand::RenderEvent, &interactiveRender);
}

//----------------------------------------------------------------------------
void vtkPVView::InteractiveRender(int pass /*=0*/)
{
  vtkVLogScopeF(
    PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: interative-render", this->GetLogName().c_str());
  const auto& internals = (*this->Internals);
  if (internals.InitializedForDataProcessing)
  {
    vtkLogF(WARNING, "InteractiveRender called on data processing service. Ignored.");
    return;
  }

  internals.LastRenderingPass = pass;
  internals.LastRenderingMode = vtkPVView::INTERACTIVE_RENDER;
}

//----------------------------------------------------------------------------
int vtkPVView::GetLastRenderingPass() const
{
  const auto& internals = (*this->Internals);
  return internals.LastRenderingPass;
}

//----------------------------------------------------------------------------
int vtkPVView::GetLastRenderingMode() const
{
  const auto& internals = (*this->Internals);
  return internals.LastRenderingMode;
}

//----------------------------------------------------------------------------
int vtkPVView::CallProcessViewRequest(vtkInformation* request)
{
  const auto& internals = (*this->Internals);
  internals.ActiveRequest = request;

  int count = 0;
  for (auto& repr : internals.Representations)
  {
    if (repr->GetVisibility())
    {
      count += repr->ProcessViewRequest(request) ? 1 : 0;
    }
  }

  internals.ActiveRequest = nullptr;
  return count;
}

//-----------------------------------------------------------------------------
void vtkPVView::SetRepresentations(const std::vector<vtkPVDataRepresentation*>& representations)
{
  const auto& internals = (*this->Internals);

  std::vector<vtkPVDataRepresentation*> to_add;
  std::vector<vtkPVDataRepresentation*> to_remove;

  const std::set<vtkPVDataRepresentation*> old_value(
    internals.Representations.begin(), internals.Representations.end());
  const std::set<vtkPVDataRepresentation*> new_value(
    representations.begin(), representations.end());

  std::set_difference(old_value.begin(), old_value.end(), new_value.begin(), new_value.end(),
    std::back_inserter(to_remove));
  std::set_difference(new_value.begin(), new_value.end(), old_value.begin(), old_value.end(),
    std::back_inserter(to_add));

  for (auto& repr : to_remove)
  {
    if (repr)
    {
      this->RemoveRepresentation(repr);
    }
  }

  for (auto& repr : to_add)
  {
    if (repr)
    {
      this->AddRepresentation(repr);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVView::AddRepresentation(vtkPVDataRepresentation* representation)
{
  auto& internals = (*this->Internals);
  auto iter =
    std::find(internals.Representations.begin(), internals.Representations.end(), representation);
  if (iter == internals.Representations.end())
  {
    if (representation->AddToView(this))
    {
      this->AddRepresentationInternal(representation);
      internals.DeliveryManager->RegisterRepresentation(representation);
      internals.Representations.push_back(representation);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVView::RemoveRepresentation(vtkPVDataRepresentation* representation)
{
  auto& internals = (*this->Internals);
  auto iter =
    std::find(internals.Representations.begin(), internals.Representations.end(), representation);
  if (iter != internals.Representations.end())
  {
    this->RemoveRepresentationInternal(representation);
    representation->RemoveFromView(this);
    internals.DeliveryManager->UnRegisterRepresentation(representation);
    internals.Representations.erase(iter);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkPVView::ScaleRendererViewports(const double viewport[4])
{
  auto window = this->GetRenderWindow();
  if (!window)
  {
    return;
  }

  assert(viewport[0] >= 0 && viewport[0] <= 1.0);
  assert(viewport[1] >= 0 && viewport[1] <= 1.0);
  assert(viewport[2] >= 0 && viewport[2] <= 1.0);
  assert(viewport[3] >= 0 && viewport[3] <= 1.0);

  auto collection = window->GetRenderers();
  collection->InitTraversal();
  while (auto renderer = collection->GetNextItem())
  {
    renderer->SetViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  }
}

//-----------------------------------------------------------------------------
void vtkPVView::SetTileScale(int x, int y)
{
  auto window = this->GetRenderWindow();
  if (window && !this->InTileDisplayMode() && !this->InCaveDisplayMode())
  {
    window->SetTileScale(x, y);
  }
}

//-----------------------------------------------------------------------------
void vtkPVView::SetTileViewport(double x0, double y0, double x1, double y1)
{
  auto window = this->GetRenderWindow();
  if (window && !this->InTileDisplayMode() && !this->InCaveDisplayMode())
  {
    window->SetTileViewport(x0, y0, x1, y1);
  }
}

//----------------------------------------------------------------------------
std::string vtkPVView::GetChannelName(vtkTypeUInt32 gid)
{
  return "view-image-" + std::to_string(gid);
}

//----------------------------------------------------------------------------
void vtkPVView::SetPiece(
  vtkPVDataRepresentation* repr, int index, vtkDataObject* data, int lodLevel)
{
  if (auto* self = repr ? repr->GetView() : nullptr)
  {
    auto& dManager = self->Internals->DeliveryManager;
    dManager->SetPiece(repr, index, data, lodLevel);
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkPVView::GetPiece(
  vtkPVDataRepresentation* repr, int index, int lodLevel)
{
  if (auto* self = repr ? repr->GetView() : nullptr)
  {
    auto& dManager = self->Internals->DeliveryManager;
    return dManager->GetPiece(repr, index, lodLevel);
  }

  return nullptr;
}

//----------------------------------------------------------------------------
vtkPVDataDeliveryManager* vtkPVView::GetDataDeliveryManager() const
{
  const auto& internals = (*this->Internals);
  return internals.DeliveryManager;
}

//----------------------------------------------------------------------------
vtkPacket vtkPVView::GetRenderedResult(vtkNJson& metadata) const
{
  vtkSynchronizedRenderers::vtkRawImage buffer;
  if (auto* window = this->GetRenderWindow())
  {
    auto* renderer = window->GetRenderers()->GetFirstRenderer();
    vtkLogStartScope(TRACE, "capture");
    buffer.Capture(renderer);
    vtkLogEndScope("capture");

    vtkNJson result;
    result["gid"] = this->GetGlobalID();
    result["width"] = buffer.GetWidth();
    result["height"] = buffer.GetHeight();
    if (!metadata.empty())
    {
      result["metadata"] = metadata;
    }

    std::map<std::string, vtkSmartPointer<vtkObject>> payload;
    payload["image"] = buffer.GetRawPtr();
    return vtkPacket(result, payload);
  }

  return vtkPacket();
}

//----------------------------------------------------------------------------
vtkPacket vtkPVView::GetRenderedResult() const
{
  vtkNJson metadata;
  return this->GetRenderedResult(metadata);
}
