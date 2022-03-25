/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMRenderViewProxy
 * @brief view proxy subclass for render view.
 *
 * vtkSMRenderViewProxy is a proxy for the standard ParaView 3D rendering view.
 *
 */

#ifndef vtkSMRenderViewProxy_h
#define vtkSMRenderViewProxy_h

#include "vtkSMViewProxy.h"

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkSMRenderViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMRenderViewProxy* New();
  vtkTypeMacro(vtkSMRenderViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum InteractionModes
  {
    INTERACTION_MODE_UNINTIALIZED = -1,
    INTERACTION_MODE_3D = 0,
    INTERACTION_MODE_2D
  };

  enum InteractionManipulatorTypes
  {
    PAN = 1,
    ZOOM = 2,
    ROLL = 3,
    ROTATE = 4,
    MULTI_ROTATE = 5,
    ZOOM_TO_MOUSE = 6,
    SKYBOX_ROTATE = 7
  };

  enum KeyManipulatorTypes
  {
    NONE = 0,
    SHIFT = 1,
    CTRL = 2
  };

  const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort) override;

protected:
  vtkSMRenderViewProxy();
  ~vtkSMRenderViewProxy();

  void InitializeInteractor(vtkRenderWindowInteractor* iren) override;
  void SetPropertyModifiedFlag(const char* name, int flag) override;

private:
  vtkSMRenderViewProxy(const vtkSMRenderViewProxy&) = delete;
  void operator=(const vtkSMRenderViewProxy&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
