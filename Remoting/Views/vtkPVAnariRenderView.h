// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVAnariRenderView
 * @brief   Anari Render View for ParaView.
 *
 * vtkPVAnariRenderView exposes vtkAnariRenderWindow as a native view type for ParaView.
 * Unlike vtkPVRenderView using vtkAnariPass, this class does not depend on an OpenGL render window,
 * but it can **only be used off-screen**.
 *
 */

#ifndef vtkPVAnariRenderView_h
#define vtkPVAnariRenderView_h

#include "vtkPVRenderView.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkPVAnariRenderView : public vtkPVRenderView
{
public:
  static vtkPVAnariRenderView* New();
  vtkTypeMacro(vtkPVAnariRenderView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetEnableANARI(bool) override;
  bool GetEnableANARI() override;

protected:
    vtkPVAnariRenderView();
};

#endif
