/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSynchronizedRenderer.cxx

  Copyright (c) Kitware, Inc.
  Copyright (c) 2017, NVIDIA CORPORATION.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSynchronizedRenderer.h"

#include "vtkBoundingBox.h"
#include "vtkCameraPass.h"
// #include "vtkCaveSynchronizedRenderers.h"
#include "vtkCompositedSynchronizedRenderers.h"
#include "vtkFXAAOptions.h"
#include "vtkImageProcessingPass.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFXAAPass.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPVDefaultPass.h"
// #include "vtkPVRenderViewSettings.h"
// #include "vtkPVServerInformation.h"
#include "vtkRenderer.h"

#if VTK_MODULE_ENABLE_ParaView_icet
#include "vtkIceTSynchronizedRenderers.h"
#endif

#include <cassert>

vtkStandardNewMacro(vtkPVSynchronizedRenderer);
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::vtkPVSynchronizedRenderer()
  : ParallelSynchronizer(nullptr)
  , ImageProcessingPass(nullptr)
  , RenderPass(nullptr)
  , Enabled(true)
  , DisableIceT(false)
  , ImageReductionFactor(1)
  , Renderer(nullptr)
  , UseDepthBuffer(false)
  , RenderEmptyImages(false)
  , DataReplicatedOnAllProcesses(false)
  , EnableRayTracing(false)
  , EnablePathTracing(false)
  , InTileDisplayMode(false)
  , InCAVEMode(false)
{
#if 0 // FIXME: ASYNC
  this->DisableIceT = vtkPVRenderViewSettings::GetInstance()->GetDisableIceT();
#endif
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::Initialize(vtkMultiProcessController* controller)
{
  assert(controller != nullptr);
  assert(this->ParallelSynchronizer == nullptr);

#if 0 // FIXME: ASYNC
  auto serverInfo = session->GetServerInformation();
  this->InTileDisplayMode = serverInfo->GetIsInTileDisplay();
  this->InCAVEMode = serverInfo->GetIsInCave();
#endif

#if VTK_MODULE_ENABLE_ParaView_icet
  if (!this->DisableIceT)
  {
    this->ParallelSynchronizer = vtkIceTSynchronizedRenderers::New();
  }
#endif
  if (this->ParallelSynchronizer == nullptr)
  {
    if (controller->GetNumberOfProcesses() > 1)
    {
      this->ParallelSynchronizer = vtkCompositedSynchronizedRenderers::New();
    }
    else
    {
      this->ParallelSynchronizer = vtkSynchronizedRenderers::New();
    }
  }

  assert(this->ParallelSynchronizer != nullptr);

  // ensure that the server ranks always render on a black background to simplify
  // compositing and avoiding issues like #15961, #18998.
  this->ParallelSynchronizer->FixBackgroundOn();
  this->ParallelSynchronizer->SetParallelController(controller);
  this->ParallelSynchronizer->SetRootProcessId(0);
  this->ParallelSynchronizer->WriteBackImagesOn();

  // calling this to ensure that the `FixBackground` state on all
  // synchronized renderers is set correctly.
  this->UpdateFixBackgroundState();
}

//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::~vtkPVSynchronizedRenderer()
{
  this->SetRenderer(nullptr);
  if (this->ParallelSynchronizer)
  {
    this->ParallelSynchronizer->Delete();
    this->ParallelSynchronizer = nullptr;
  }

  this->SetImageProcessingPass(nullptr);
  this->SetRenderPass(nullptr);
  this->SetFXAAOptions(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetImageProcessingPass(vtkImageProcessingPass* pass)
{
  if (this->ImageProcessingPass == pass)
  {
    return;
  }
  vtkSetObjectBodyMacro(ImageProcessingPass, vtkImageProcessingPass, pass);
  this->SetupPasses();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetRenderPass(vtkRenderPass* pass)
{
  if (this->RenderPass == pass)
  {
    return;
  }

  vtkSetObjectBodyMacro(RenderPass, vtkRenderPass, pass);
  this->SetupPasses();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetUseDepthBuffer(bool useDB)
{
  if (this->ParallelSynchronizer == nullptr)
  {
    return;
  }

#if VTK_MODULE_ENABLE_ParaView_icet
  if (this->ParallelSynchronizer->IsA("vtkIceTSynchronizedRenderers") == 1)
  {
    vtkIceTSynchronizedRenderers* aux = (vtkIceTSynchronizedRenderers*)this->ParallelSynchronizer;
    aux->SetUseDepthBuffer(useDB);
  }
#else
  static_cast<void>(useDB);  // unused warning when MPI is off.
#endif
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetRenderEmptyImages(bool useREI)
{
  if (this->ParallelSynchronizer == nullptr)
  {
    return;
  }
#if VTK_MODULE_ENABLE_ParaView_icet
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
  {
    sync->SetRenderEmptyImages(useREI);
  }
#else
  static_cast<void>(useREI); // unused warning when MPI is off.
#endif
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetupPasses()
{
#if VTK_MODULE_ENABLE_ParaView_icet
  vtkIceTSynchronizedRenderers* iceTRen =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (iceTRen)
  {
    iceTRen->SetRenderPass(this->RenderPass);
    iceTRen->SetImageProcessingPass(this->ImageProcessingPass);
    return;
  }
#endif

  if (!this->Renderer)
  {
    return;
  }
  vtkNew<vtkOpenGLFXAAPass> fxaaPass;
  fxaaPass->SetFXAAOptions(this->GetFXAAOptions());
  vtkNew<vtkCameraPass> cameraPass;
  if (this->GetUseFXAA())
  {
    this->Renderer->SetPass(fxaaPass);
  }
  if (this->ImageProcessingPass)
  {
    if (this->GetUseFXAA())
    {
      fxaaPass->SetDelegatePass(this->ImageProcessingPass);
    }
    else
    {
      this->Renderer->SetPass(this->ImageProcessingPass);
    }
    this->ImageProcessingPass->SetDelegatePass(cameraPass);
  }
  else
  {
    this->Renderer->SetPass(cameraPass);
  }

  if (this->RenderPass)
  {
    cameraPass->SetDelegatePass(this->RenderPass);
  }
  else
  {
    vtkNew<vtkPVDefaultPass> defaultPass;
    cameraPass->SetDelegatePass(defaultPass);
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetEnabled(bool enabled)
{
  this->ParallelSynchronizer->SetParallelRendering(enabled);
  this->Enabled = enabled;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetRenderer(vtkRenderer* ren)
{
  this->ParallelSynchronizer->SetRenderer(ren);

  // The renderer should be OpenGL ...
  vtkOpenGLRenderer* glRenderer = vtkOpenGLRenderer::SafeDownCast(ren);

  if (ren && !glRenderer)
  {
    // BUG# 13567. It's not a critical error if the renderer is not a
    // vtkOpenGLRenderer. We just don't support any render-pass stuff for such
    // renderers.
    vtkDebugMacro("Received non OpenGL renderer");
  }

  vtkSetObjectBodyMacro(Renderer, vtkOpenGLRenderer, glRenderer);
  this->SetupPasses();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetImageReductionFactor(int factor)
{
  if (this->ImageReductionFactor == factor || factor < 1 || factor > 50)
  {
    return;
  }

  this->ImageReductionFactor = factor;
  if (this->ParallelSynchronizer)
  {
    this->ParallelSynchronizer->SetImageReductionFactor(this->ImageReductionFactor);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetDataReplicatedOnAllProcesses(bool replicated)
{
#if VTK_MODULE_ENABLE_ParaView_icet
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
  {
    sync->SetDataReplicatedOnAllProcesses(replicated);
  }
#endif
  this->DataReplicatedOnAllProcesses = replicated;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetOrderedCompositingHelper(vtkOrderedCompositingHelper* helper)
{
#if VTK_MODULE_ENABLE_ParaView_icet
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
  {
    sync->SetOrderedCompositingHelper(helper);
    sync->SetUseOrderedCompositing(helper != nullptr);
  }
#endif
  (void)helper;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetEnableRayTracing(bool val)
{
  if (this->EnableRayTracing != val)
  {
    this->EnableRayTracing = val;
    this->Modified();
    this->UpdateFixBackgroundState();
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetEnablePathTracing(bool val)
{
  if (this->EnablePathTracing != val)
  {
    this->EnablePathTracing = val;
    this->Modified();
    this->UpdateFixBackgroundState();
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::UpdateFixBackgroundState()
{
  // note EnablePathTracing==true has no effect unless EnableRayTracing==true as well.
  const bool fix_background = !(this->EnableRayTracing && this->EnablePathTracing);
  this->ParallelSynchronizer->SetFixBackground(fix_background);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetUseFXAA(bool use)
{
  if (this->UseFXAA == use)
  {
    return;
  }
  this->UseFXAA = use;
  this->SetupPasses();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetFXAAOptions(vtkFXAAOptions* options)
{
  if (this->FXAAOptions == options)
  {
    return;
  }
  vtkSetObjectBodyMacro(FXAAOptions, vtkFXAAOptions, options);
  this->SetupPasses();
}
