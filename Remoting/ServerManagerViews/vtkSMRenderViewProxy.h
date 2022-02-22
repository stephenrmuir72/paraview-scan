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

  const char* GetRepresentationType(vtkSMSourceProxy* producer, int outputPort) override;

protected:
  vtkSMRenderViewProxy();
  ~vtkSMRenderViewProxy();

private:
  vtkSMRenderViewProxy(const vtkSMRenderViewProxy&) = delete;
  void operator=(const vtkSMRenderViewProxy&) = delete;
};

#endif
