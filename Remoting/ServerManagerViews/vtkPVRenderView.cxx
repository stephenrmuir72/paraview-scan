/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.cxx

  Copyright (c) Kitware, Inc.
  Copyright (c) 2017, NVIDIA CORPORATION.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderView.h"

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVMaterialLibrary.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSkybox.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTuple.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayLightNode.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#endif

//-----------------------------------------------------------------------------
namespace
{
void PrepareTexture(vtkTexture* texture)
{
  // This is really bad! We're modifying the input! But preserving what the
  // old code did for now. Need to be investigated.
  if (texture != nullptr)
  {
    // environment texture is always declared in linear color space
    vtkImageData* input = texture->GetInput();
    if (input)
    {
      texture->Update();
      texture->SetUseSRGBColorSpace(input->GetScalarType() == VTK_UNSIGNED_CHAR);

      // mip map is required for correct IBL generation
      texture->MipmapOn();
      texture->InterpolateOn();
    }
  }
}
}

//-----------------------------------------------------------------------------
class vtkPVRenderView::vtkRInternals
{
public:
  vtkSmartPointer<vtkRenderWindow> RenderWindow;
  vtkNew<vtkRenderer> Renderers[2];
  vtkNew<vtkCamera> Camera;
  vtkNew<vtkPVAxesWidget> OrientationWidget;
  vtkNew<vtkPVSynchronizedRenderer> SynchronizedRenderers;
  vtkSmartPointer<vtkRenderPass> DefaultRenderPass;

  bool UseRenderViewSettingsForBackground{ true };
  int BackgroundColorMode{ vtkPVRenderView::DEFAULT };
  vtkTuple<double, 3> Background{ { 0, 0, 0 } };
  vtkTuple<double, 3> Background2{ { 0, 0, 0 } };
  vtkSmartPointer<vtkTexture> EnvironmentalBGTexture;
  bool UseEnvironmentLighting{ false };
  bool UseTexturedEnvironmentalBG{ false };
  vtkNew<vtkSkybox> Skybox;

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkSmartPointer<vtkOSPRayPass> OSPRayPass;
  bool UsingRayTracing{ false };
#endif

  void UpdateBackground(vtkRenderer* renderer) const;
};

//-----------------------------------------------------------------------------
void vtkPVRenderView::vtkRInternals::UpdateBackground(vtkRenderer* renderer) const
{
  vtkTuple<double, 3> color = this->Background;
  vtkTuple<double, 3> color2 = this->Background2;
  int mode = this->BackgroundColorMode;

  if (this->UseRenderViewSettingsForBackground)
  {
    auto settings = vtkPVRenderViewSettings::GetInstance();
    mode = settings->GetBackgroundColorMode();
    settings->GetBackgroundColor(color.GetData());
    settings->GetBackground2Color(color2.GetData());
  }

  switch (mode)
  {
    case DEFAULT:
      renderer->SetTexturedBackground(false);
      renderer->SetGradientBackground(false);
      renderer->SetUseImageBasedLighting(this->UseTexturedEnvironmentalBG);
      break;

    case GRADIENT:
      renderer->SetTexturedBackground(false);
      renderer->SetGradientBackground(true);
      renderer->SetUseImageBasedLighting(false);
      break;

    case IMAGE:
      renderer->SetTexturedBackground(true);
      renderer->SetGradientBackground(false);
      renderer->SetUseImageBasedLighting(this->UseEnvironmentLighting);
      break;

    case SKYBOX:
      renderer->SetTexturedBackground(false);
      renderer->SetGradientBackground(false);
      renderer->SetUseImageBasedLighting(this->UseEnvironmentLighting);
      break;

    case STEREO_SKYBOX:
      renderer->SetTexturedBackground(false);
      renderer->SetGradientBackground(false);
      renderer->SetUseImageBasedLighting(this->UseEnvironmentLighting);
      break;

    default:
      break;
  }

  renderer->SetBackground(color.GetData());
  renderer->SetBackground2(color2.GetData());

  // update skybox texture.
  vtkTexture* texture = renderer->GetBackgroundTexture();
  if ((this->BackgroundColorMode == vtkPVRenderView::STEREO_SKYBOX ||
        this->BackgroundColorMode == vtkPVRenderView::SKYBOX) &&
    texture != nullptr)
  {
    ::PrepareTexture(texture);

    this->Skybox->SetProjection(this->BackgroundColorMode == vtkPVRenderView::SKYBOX
        ? vtkSkybox::Sphere
        : vtkSkybox::StereoSphere);
    // Update skybox orientation from renderer orientation
    double* up = renderer->GetEnvironmentUp();
    double* right = renderer->GetEnvironmentRight();
    double front[3];
    vtkMath::Cross(right, up, front);
    this->Skybox->SetFloorRight(front[0], front[1], front[2]);

    this->Skybox->SetTexture(texture);
    this->Skybox->SetVisibility(1);
    renderer->SetEnvironmentTexture(texture);
  }
  else
  {
    this->Skybox->SetVisibility(0);
    renderer->SetEnvironmentTexture(this->EnvironmentalBGTexture);
  }
}

vtkStandardNewMacro(vtkPVRenderView);
//-----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView() = default;

//-----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView() = default;

//-----------------------------------------------------------------------------
void vtkPVRenderView::InitializeForDataProcessing()
{
  this->Superclass::InitializeForDataProcessing();
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::InitializeForRendering()
{
  this->Superclass::InitializeForRendering();

  this->RInternals.reset(new vtkPVRenderView::vtkRInternals());
  auto& internals = (*this->RInternals);
  internals.RenderWindow = vtk::TakeSmartPointer(this->NewRenderWindow());
  internals.RenderWindow->SetNumberOfLayers(3);
  internals.RenderWindow->SetMultiSamples(0);

  // 3D renderer
  internals.Renderers[0]->SetActiveCamera(internals.Camera);

  // add skybox
  internals.Renderers[0]->AddActor(internals.Skybox);
  internals.Skybox->SetVisibility(0);

  // 2D renderer
  internals.Renderers[1]->SetLayer(2);
  internals.Renderers[1]->EraseOff();
  internals.Renderers[1]->InteractiveOff();
  internals.Renderers[1]->SetActiveCamera(internals.Camera);

  internals.RenderWindow->AddRenderer(internals.Renderers[0]);
  internals.RenderWindow->AddRenderer(internals.Renderers[1]);

  // Setup distributed rendering, if needed.
  auto* controller = this->GetController();
  internals.SynchronizedRenderers->Initialize(controller);
  internals.SynchronizedRenderers->SetRenderer(internals.Renderers[0]);
  internals.DefaultRenderPass = internals.SynchronizedRenderers->GetRenderPass();

  // Setup OrientationWidget
  internals.OrientationWidget->SetParentRenderer(internals.Renderers[0]);
  internals.OrientationWidget->SetViewport(0, 0, 0.25, 0.25);
}

//-----------------------------------------------------------------------------
vtkRenderer* vtkPVRenderView::GetRenderer(int rendererType /*= DEFAULT_RENDERER*/) const
{
  assert(this->RInternals);
  const auto& rinternals = (*this->RInternals);
  return rinternals.Renderers[rendererType];
}

//-----------------------------------------------------------------------------
vtkRenderWindow* vtkPVRenderView::GetRenderWindow() const
{
  assert(this->RInternals);
  const auto& rinternals = (*this->RInternals);
  return rinternals.RenderWindow;
}

//-----------------------------------------------------------------------------
vtkCamera* vtkPVRenderView::GetActiveCamera() const
{
  assert(this->RInternals);
  const auto& rinternals = (*this->RInternals);
  return rinternals.Camera;
}

//-----------------------------------------------------------------------------
vtkBoundingBox vtkPVRenderView::GetVisiblePropBounds()
{
  if (auto* renderer = this->GetRenderer())
  {
    double bounds[6];
    renderer->ComputeVisiblePropBounds(bounds);

    vtkBoundingBox bbox;
    bbox.SetBounds(bounds);

    auto* controller = this->GetController();
    controller->Reduce(bbox, bbox, 0);
    if (controller->GetLocalProcessId() == 0)
    {
      return bbox;
    }
  }

  return vtkBoundingBox();
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetEnableRayTracing(bool enableRayTracing)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  if (enableRayTracing && !rinternals.OSPRayPass)
  {
    rinternals.OSPRayPass = vtk::TakeSmartPointer(vtkOSPRayPass::New());
  }
  if (enableRayTracing && !vtkOSPRayPass::IsSupported())
  {
    enableRayTracing = false;
    vtkLogF(ERROR, "RayTracing cannot be enabled since not supported by the runtime environment!");
  }

  if (!enableRayTracing)
  {
    rinternals.SynchronizedRenderers->SetRenderPass(rinternals.DefaultRenderPass);
    rinternals.SynchronizedRenderers->SetEnableRayTracing(false);
  }
  else
  {
    rinternals.SynchronizedRenderers->SetRenderPass(rinternals.OSPRayPass);
    rinternals.SynchronizedRenderers->SetEnableRayTracing(true);
  }
  rinternals.UsingRayTracing = enableRayTracing;
#else
  if (enableRayTracing)
  {
    vtkLogF(ERROR,
      "Ray-tracing cannot be enabled since it's not available in"
      " this build.");
  }
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetRayTracingBackEnd(const std::string& backend)
{
  (void)backend;
  assert(this->RInternals);
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetRendererType(backend, this->GetRenderer());

  const bool pathtrace = (backend.find(std::string("pathtracer")) != std::string::npos);
  const auto& rinternals = (*this->RInternals);
  rinternals.SynchronizedRenderers->SetEnablePathTracing(pathtrace);
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetEnableShadows(bool enableShadows)
{
  this->GetRenderer()->SetUseShadows(enableShadows);
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetAmbientOcclusionSamples(int samples)
{
  (void)samples;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetAmbientSamples(samples, this->GetRenderer());
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetNumberSamplesPerPixel(int samples)
{
  (void)samples;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetSamplesPerPixel(samples, this->GetRenderer());
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetNumberOfProgressivePasses(int passes)
{
  (void)passes;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetMaxFrames(passes, this->GetRenderer());
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetRouletteDepth(int depth)
{
  (void)depth;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetRouletteDepth(depth, this->GetRenderer());
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetVolumeAnisotropy(double value)
{
  (void)value;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetVolumeAnisotropy(value, this->GetRenderer());
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetEnableDenoise(bool enableDenoise)
{
  (void)enableDenoise;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetEnableDenoiser(enableDenoise, this->GetRenderer());
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetLightScale(double scale)
{
  (void)scale;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayLightNode::SetLightScale(scale);
#endif
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetMaterialLibrary(vtkPVMaterialLibrary* library)
{
  (void)library;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetMaterialLibrary(
    library ? vtkOSPRayMaterialLibrary::SafeDownCast(library->GetMaterialLibrary()) : nullptr,
    this->GetRenderer());
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackgroundMode(int val)
{
  (void)val;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayRendererNode::SetBackgroundMode(
    static_cast<vtkOSPRayRendererNode::BackgroundMode>(val), this->GetRenderer());
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackgroundNorth(double x, double y, double z)
{
  (void)x;
  (void)y;
  (void)z;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  double dir[3] = { x, y, z };
  vtkOSPRayRendererNode::SetNorthPole(dir, this->GetRenderer());
#endif
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetBackgroundEast(double x, double y, double z)
{
  (void)x;
  (void)y;
  (void)z;
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  double dir[3] = { x, y, z };
  vtkOSPRayRendererNode::SetEastPole(dir, this->GetRenderer());
#endif
}

//-----------------------------------------------------------------------------
bool vtkPVRenderView::GetContinueRendering() const
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  assert(this->RInternals);
  const auto& rinternals = (*this->RInternals);
  if (rinternals.UsingRayTracing)
  {
    return (this->GetLastRenderingPass() + 1) <
      vtkOSPRayRendererNode::GetMaxFrames(this->GetRenderer());
  }
#endif
  return this->Superclass::GetContinueRendering();
}

//-----------------------------------------------------------------------------
int vtkPVRenderView::GetNumberOfProgressivePasses() const
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  return vtkOSPRayRendererNode::GetMaxFrames(this->GetRenderer());
#else
  return 0;
#endif
}

//-----------------------------------------------------------------------------
bool vtkPVRenderView::IsRayTracingEnabled() const
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  assert(this->RInternals);
  const auto& rinternals = (*this->RInternals);
  return rinternals.UsingRayTracing;
#endif
  return false;
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetUseRenderViewSettingsForBackground(bool value)
{
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  rinternals.UseRenderViewSettingsForBackground = value;
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetBackgroundColorMode(int value)
{
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  rinternals.BackgroundColorMode = value;
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetBackground(double r, double g, double b)
{
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  rinternals.Background = vtkTuple<double, 3>({ r, g, b });
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetBackground2(double r, double g, double b)
{
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  rinternals.Background2 = vtkTuple<double, 3>({ r, g, b });
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::SetUseEnvironmentLighting(bool value)
{
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  rinternals.UseEnvironmentLighting = value;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetEnvironmentalBG(double r, double g, double b)
{
  this->GetRenderer()->SetEnvironmentalBG(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetEnvironmentalBG2(double r, double g, double b)
{
  this->GetRenderer()->SetEnvironmentalBG2(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetEnvironmentalBGTexture(vtkTexture* texture)
{
  ::PrepareTexture(texture);
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  rinternals.EnvironmentalBGTexture = texture;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetGradientEnvironmentalBG(int val)
{
  this->GetRenderer()->SetGradientEnvironmentalBG(val ? true : false);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetTexturedEnvironmentalBG(int val)
{
  assert(this->RInternals);
  auto& rinternals = (*this->RInternals);
  rinternals.UseTexturedEnvironmentalBG = (val ? true : false);
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();
  assert(this->RInternals);
  const auto& rinternals = (*this->RInternals);
  rinternals.UpdateBackground(this->GetRenderer());
}

//-----------------------------------------------------------------------------
void vtkPVRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
