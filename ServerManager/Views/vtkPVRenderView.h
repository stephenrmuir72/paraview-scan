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

#include "vtkPVView.h"
#include "vtkServerManagerViewsModule.h" //needed for exports

class vtkCamera;
class vtkRenderer;

class VTKSERVERMANAGERVIEWS_EXPORT vtkPVRenderView : public vtkPVView
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
  ;
  void InitializeForRendering() override;
  ///@}

protected:
  vtkPVRenderView();
  ~vtkPVRenderView() override;

private:
  vtkPVRenderView(const vtkPVRenderView&) = delete;
  void operator=(const vtkPVRenderView&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
