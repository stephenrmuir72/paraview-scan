/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVRenderView
 * @brief   Render View for ParaView.
 *
 * vtkRenderView equivalent that is specialized for ParaView. vtkRenderView
 * handles polygonal rendering for ParaView in all the different modes of
 * operation. vtkPVRenderView instance must be created on all involved
 * processes. vtkPVRenderView uses the information about what process it has
 * been created on to decide what part of the "rendering" happens on the
 * process.
 */

#ifndef vtkPVRenderView_h
#define vtkPVRenderView_h

#include "vtkBoundingBox.h" // for vtkBoundingBox
#include "vtkPVView.h"
#include "vtkRemotingServerManagerViewsModule.h" //needed for exports

class vtkCamera;
class vtkPVMaterialLibrary;
class vtkRenderer;
class vtkTexture;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkPVRenderView : public vtkPVView
{
  //*****************************************************************
public:
  static vtkPVRenderView* New();
  vtkTypeMacro(vtkPVRenderView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Defines various renderer types.
   */
  enum
  {
    DEFAULT_RENDERER = 0,
    NON_COMPOSITED_RENDERER = 1,
  };

  ///@{
  /**
   * Gets the non-composited renderer for this view. This is typically used for
   * labels, 2D annotations etc.
   * \note CallOnAllProcesses
   */
  vtkRenderer* GetRenderer() const { return this->GetRenderer(DEFAULT_RENDERER); }
  vtkRenderer* GetNonCompositedRenderer() const
  {
    return this->GetRenderer(NON_COMPOSITED_RENDERER);
  }
  vtkRenderer* GetRenderer(int rendererType) const;
  ///@}

  ///@{
  // vtkPVView API overrides
  vtkCamera* GetActiveCamera() const override;
  vtkRenderWindow* GetRenderWindow() const override;
  void InitializeForDataProcessing() override;
  void InitializeForRendering() override;
  ///@}

  /**
   * Computes visible prop bounds.
   */
  virtual vtkBoundingBox GetVisiblePropBounds();

  ///@{
  /**
   * Ray-tracing related properties
   */
  void SetEnableRayTracing(bool enableRayTracing);
  bool IsRayTracingEnabled() const;
  void SetRayTracingBackEnd(const std::string& backend);
  void SetEnableShadows(bool enableShadows);
  void SetAmbientOcclusionSamples(int samples);
  void SetNumberSamplesPerPixel(int samples);
  void SetNumberOfProgressivePasses(int passes);
  int GetNumberOfProgressivePasses() const;
  void SetRouletteDepth(int depth);
  void SetVolumeAnisotropy(double value);
  void SetEnableDenoise(bool enableDenoise);
  void SetLightScale(double scale);
  void SetMaterialLibrary(vtkPVMaterialLibrary* library);
  void SetBackgroundMode(int val);
  void SetBackgroundNorth(double x, double y, double z);
  void SetBackgroundEast(double x, double y, double z);
  ///@}

  /**
   * Overridden to return true if this->GetLastRenderingPass() < number of
   * progressive passes.
   */
  bool GetContinueRendering() const override;

  enum BackgroundColorModes
  {
    DEFAULT,
    GRADIENT,
    IMAGE,
    SKYBOX,
    STEREO_SKYBOX,
  };

  ///@{
  /**
   * API for background color selection.
   */
  void SetUseRenderViewSettingsForBackground(bool value);
  void SetBackgroundColorMode(int value);
  void SetBackground(double, double, double);
  void SetBackground2(double, double, double);
  void SetUseEnvironmentLighting(bool value);
  ///@}

  //*****************************************************************
  // Entry point for environmental backgrounds
  void SetEnvironmentalBG(double r, double g, double b);
  void SetEnvironmentalBG2(double r, double g, double b);
  void SetEnvironmentalBGTexture(vtkTexture* val);
  void SetGradientEnvironmentalBG(int val);
  void SetTexturedEnvironmentalBG(int val);

protected:
  vtkPVRenderView();
  ~vtkPVRenderView() override;

  /**
   * Overridden to setup background etc. if needed.
   */
  void PrepareForRendering() override;

private:
  vtkPVRenderView(const vtkPVRenderView&) = delete;
  void operator=(const vtkPVRenderView&) = delete;

  class vtkRInternals;
  std::unique_ptr<vtkRInternals> RInternals;
};

#endif
