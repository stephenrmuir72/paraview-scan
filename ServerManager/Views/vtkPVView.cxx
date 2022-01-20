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

#include "vtkBoundingBox.h"
#include "vtkCommunicator.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVLogger.h"
#include "vtkPVProcessWindow.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
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

vtkInformationKeyMacro(vtkPVView, REQUEST_RENDER, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_UPDATE_LOD, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_UPDATE, Request);
vtkInformationKeyRestrictedMacro(vtkPVView, VIEW, ObjectBase, "vtkPVView");
//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkPVView, RenderWindow, vtkRenderWindow);
//----------------------------------------------------------------------------
vtkPVView::vtkPVView(bool create_render_window)
{
  // Ensure vtkProcessModule is setup correctly.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkErrorMacro("vtkProcessModule not initialized. Aborting.");
    abort();
  }

  this->ViewTime = 0.0;
  this->CacheKey = 0.0;
  this->UseCache = false;

  this->RequestInformation = vtkInformation::New();
  this->ReplyInformationVector = vtkInformationVector::New();

  this->ViewTimeValid = false;

  this->Size[1] = this->Size[0] = 300;
  this->Position[0] = this->Position[1] = 0;
  this->PPI = 72;

  this->InCaptureScreenshot = false;

  // Create render window, if requested.
  this->RenderWindow = create_render_window ? this->NewRenderWindow() : nullptr;
  if (this->RenderWindow)
  {
    this->RenderWindow->SetSize(this->Size);
    this->RenderWindow->SetPosition(this->Position);
    this->RenderWindow->SetDPI(this->PPI);
  }

  this->DeliveryManager = nullptr;
}

//----------------------------------------------------------------------------
vtkPVView::~vtkPVView()
{
  this->SetDeliveryManager(nullptr);
  this->RequestInformation->Delete();
  this->ReplyInformationVector->Delete();
  this->SetRenderWindow(nullptr);
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVView::NewRenderWindow()
{
  auto pm = vtkProcessModule::GetProcessModule();
  auto config = vtkRemotingCoreConfiguration::GetInstance();

  // A little bit of a hack. I am not what's a good place to setup the display
  // environment for this rank. So doing it here.
  config->HandleDisplayEnvironment();

  vtkSmartPointer<vtkRenderWindow> window;
  switch (pm->GetProcessType())
  {
    case vtkProcessModule::PROCESS_CLIENT:
      if (vtkPVView::GetUseGenericOpenGLRenderWindow())
      {
        window = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
      }
      else if (config->GetForceOffscreenRendering())
      {
        // this may be a headless window if ParaView was built with headless
        // capabilities.
        window = vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();
      }
      else
      {
        window = vtkSmartPointer<vtkRenderWindow>::New();
      }
      break;

    case vtkProcessModule::PROCESS_SERVER:
    case vtkProcessModule::PROCESS_BATCH:
      if (auto processWindow = vtkPVProcessWindow::GetRenderWindow())
      {
        auto owindow = vtkSmartPointer<vtkOffscreenOpenGLRenderWindow>::New();
        owindow->SetContext(vtkOpenGLRenderWindow::SafeDownCast(processWindow));
        owindow->DoubleBufferOff();
        window = owindow;
      }
      else
      {
        vtkErrorMacro(
          "Expecting a process window, which is missing. Aborting for debugging purposes.");
        abort();
      }
      break;

    default:
      vtkErrorMacro("Invalid process type. Aborting for debugging purposes.");
      abort();
  }

  if (window)
  {
    vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(),
      "created a `%s` as a new render window for view %s", vtkLogIdentifier(window),
      vtkLogIdentifier(this));

    window->AlphaBitPlanesOn();

    std::ostringstream str;
    switch (pm->GetProcessType())
    {
      case vtkProcessModule::PROCESS_SERVER:
        str << "ParaView Server";
        break;
      case vtkProcessModule::PROCESS_CLIENT:
      case vtkProcessModule::PROCESS_BATCH:
      default:
        str << "ParaView";
        break;
    }
    if (pm->GetNumberOfRanks() > 1)
    {
      str << pm->GetRank();
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
void vtkPVView::SetPosition(int x, int y)
{
  this->Position[0] = x;
  this->Position[1] = y;
}

//----------------------------------------------------------------------------
void vtkPVView::SetSize(int x, int y)
{
  if (this->InTileDisplayMode() || this->InCaveDisplayMode())
  {
    // the size request is ignored.
  }
  else if (auto window = this->GetRenderWindow())
  {
    const auto cur_size = window->GetActualSize();
    if (cur_size[0] != x || cur_size[1] != y)
    {
      window->SetSize(x, y);
    }
  }
  this->Size[0] = x;
  this->Size[1] = y;
}

//----------------------------------------------------------------------------
void vtkPVView::SetPPI(int ppi)
{
  this->PPI = ppi;
  if (auto window = this->GetRenderWindow())
  {
    window->SetDPI(ppi);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVView::SetViewTime(double time)
{
  if (!this->ViewTimeValid || this->ViewTime != time)
  {
    this->ViewTime = time;
    this->ViewTimeValid = true;
    this->Modified();
  }
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
bool vtkPVView::GetLocalProcessSupportsInteraction()
{
  auto pm = vtkProcessModule::GetProcessModule();
  switch (pm->GetProcessType())
  {
    case vtkProcessModule::PROCESS_CLIENT:
      assert(pm->GetRank() == 0);
      return true;

    case vtkProcessModule::PROCESS_BATCH:
      return (pm->GetRank() == 0);

    case vtkProcessModule::PROCESS_SERVER:
    default:
      return false;
  }
}

//----------------------------------------------------------------------------
void vtkPVView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ViewTime: " << this->ViewTime << endl;
  os << indent << "CacheKey: " << this->CacheKey << endl;
  os << indent << "UseCache: " << this->UseCache << endl;
}

//----------------------------------------------------------------------------
void vtkPVView::PrepareForScreenshot()
{
  this->InCaptureScreenshot = true;
}

//----------------------------------------------------------------------------
void vtkPVView::CleanupAfterScreenshot()
{
  this->InCaptureScreenshot = false;
}

//----------------------------------------------------------------------------
void vtkPVView::Update()
{
  vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: update view", this->GetLogName().c_str());

  // Propagate update time.
  const int num_reprs = this->GetNumberOfRepresentations();
  const auto view_time = this->GetViewTime();
  for (int cc = 0; cc < num_reprs; cc++)
  {
    if (auto pvrepr = vtkPVDataRepresentation::SafeDownCast(this->GetRepresentation(cc)))
    {
      // Pass the view time information to the representation
      if (this->ViewTimeValid)
      {
        pvrepr->SetUpdateTime(view_time);
      }
      else
      {
        pvrepr->ResetUpdateTime();
      }
    }
  }

  vtkTimerLog::MarkStartEvent("vtkPVView::Update");
  const int count = this->CallProcessViewRequest(
    vtkPVView::REQUEST_UPDATE(), this->RequestInformation, this->ReplyInformationVector);
  vtkTimerLog::MarkEndEvent("vtkPVView::Update");
  this->UpdateTimeStamp.Modified();
}

//----------------------------------------------------------------------------
int vtkPVView::CallProcessViewRequest(
  vtkInformationRequestKey* type, vtkInformation* inInfo, vtkInformationVector* outVec)
{
  int count = 0;
  int num_reprs = this->GetNumberOfRepresentations();
  outVec->SetNumberOfInformationObjects(num_reprs);

  // NOTE: This will create a reference loop (depending on what inInfo is). If
  // it's this->RequestInformation, then we have a loop and hence it's
  // essential to call vtkInformation::Clear() before this method returns.
  inInfo->Set(VIEW(), this);

  for (int cc = 0; cc < num_reprs; cc++)
  {
    vtkInformation* outInfo = outVec->GetInformationObject(cc);
    outInfo->Clear();
    vtkDataRepresentation* repr = this->GetRepresentation(cc);
    vtkPVDataRepresentation* pvrepr = vtkPVDataRepresentation::SafeDownCast(repr);
    if (pvrepr)
    {
      if (pvrepr->GetVisibility())
      {
        if (pvrepr->ProcessViewRequest(type, inInfo, outInfo))
        {
          ++count;
        }
      }
    }
    else if (repr && type == REQUEST_UPDATE())
    {
      repr->Update();
    }
  }

  // Clear input information since we are done with the pass. This avoids any
  // need for garbage collection.
  inInfo->Clear();
  return count;
}

//-----------------------------------------------------------------------------
void vtkPVView::AddRepresentationInternal(vtkDataRepresentation* rep)
{
  if (vtkPVDataRepresentation* drep = vtkPVDataRepresentation::SafeDownCast(rep))
  {
    if (drep->GetView() != this)
    {
      vtkErrorMacro(<< drep->GetClassName()
                    << " may not be calling this->Superclass::AddToView(...) in its "
                    << "AddToView implementation. Please fix that. "
                    << "Also check the same for RemoveFromView(..).");
    }

    if (this->DeliveryManager)
    {
      this->DeliveryManager->RegisterRepresentation(drep);
    }
  }
  this->Superclass::AddRepresentationInternal(rep);
}

//----------------------------------------------------------------------------
void vtkPVView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{
  if (auto dataRep = vtkPVDataRepresentation::SafeDownCast(rep))
  {
    if (this->DeliveryManager)
    {
      this->DeliveryManager->UnRegisterRepresentation(dataRep);
    }
  }
  this->Superclass::RemoveRepresentationInternal(rep);
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
bool vtkPVView::IsCached(vtkPVDataRepresentation* repr)
{
  if (this->DeliveryManager && this->DeliveryManager->HasPiece(repr))
  {
    vtkLogF(TRACE, "cached %s", repr->GetLogName().c_str());
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPVView::ClearCache(vtkPVDataRepresentation* repr)
{
  assert(repr != nullptr);
  if (!this->GetUseCache() && !repr->GetForceUseCache() && this->DeliveryManager != nullptr)
  {
    vtkLogF(TRACE, "clear cache %s", repr->GetLogName().c_str());
    this->DeliveryManager->ClearCache(repr);
  }
}

//-----------------------------------------------------------------------------
void vtkPVView::SetDeliveryManager(vtkPVDataDeliveryManager* manager)
{
  if (this->DeliveryManager)
  {
    this->DeliveryManager->SetView(nullptr);
  }
  vtkSetObjectBodyMacro(DeliveryManager, vtkPVDataDeliveryManager, manager);
  if (this->DeliveryManager)
  {
    // not reference counted.
    this->DeliveryManager->SetView(this);
  }
}

//-----------------------------------------------------------------------------
vtkPVDataDeliveryManager* vtkPVView::GetDeliveryManager(vtkInformation* info)
{
  auto* view = info ? vtkPVView::SafeDownCast(info->Get(VIEW())) : nullptr;
  if (!view)
  {
    vtkGenericWarningMacro("Missing VIEW().");
    return nullptr;
  }
  return view->GetDeliveryManager();
}

//-----------------------------------------------------------------------------
void vtkPVView::SetPiece(vtkInformation* info, vtkPVDataRepresentation* repr, vtkDataObject* data,
  unsigned long trueSize, int port)
{
  if (auto dm = vtkPVView::GetDeliveryManager(info))
  {
    dm->SetPiece(repr, data, false, trueSize, port);
  }
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkPVView::GetPiece(vtkInformation* info, vtkPVDataRepresentation* repr, int port)
{
  if (auto dm = vtkPVView::GetDeliveryManager(info))
  {
    return dm->GetPiece(repr, false, port);
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkPVView::GetDeliveredPiece(
  vtkInformation* info, vtkPVDataRepresentation* repr, int port)
{
  if (auto dm = vtkPVView::GetDeliveryManager(info))
  {
    return dm->GetDeliveredPiece(repr, false, port);
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void vtkPVView::SetPieceLOD(vtkInformation* info, vtkPVDataRepresentation* repr,
  vtkDataObject* data, unsigned long trueSize, int port)
{
  if (auto dm = vtkPVView::GetDeliveryManager(info))
  {
    dm->SetPiece(repr, data, true, trueSize, port);
  }
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkPVView::GetPieceLOD(vtkInformation* info, vtkPVDataRepresentation* repr, int port)
{
  if (auto dm = vtkPVView::GetDeliveryManager(info))
  {
    return dm->GetPiece(repr, true, port);
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkPVView::GetDeliveredPieceLOD(
  vtkInformation* info, vtkPVDataRepresentation* repr, int port)
{
  if (auto dm = vtkPVView::GetDeliveryManager(info))
  {
    return dm->GetDeliveredPiece(repr, true, port);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVView::Deliver(int use_lod, unsigned int size, unsigned int* representation_ids)
{
  if (auto dm = this->GetDeliveryManager())
  {
    dm->Deliver(use_lod, size, representation_ids);
  }
}
