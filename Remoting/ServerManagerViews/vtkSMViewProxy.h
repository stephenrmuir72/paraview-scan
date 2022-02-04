/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMViewProxy
 * @brief
 *
 */

#ifndef vtkSMViewProxy_h
#define vtkSMViewProxy_h

#include "vtkRemotingServerManagerViewsModule.h" // for exports
#include "vtkSMProxy.h"

class vtkRenderWindow;
class vtkPacket;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkSMViewProxy : public vtkSMProxy
{
public:
  static vtkSMViewProxy* New();
  vtkTypeMacro(vtkSMViewProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the render window.
   */
  vtkRenderWindow* GetRenderWindow() const;

  /**
   * Called by vtkProcessInitializer when a rendering result is available.
   */
  void PostRenderingResult(const vtkPacket& packet);

protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy() override;

private:
  vtkSMViewProxy(const vtkSMViewProxy&) = delete;
  void operator=(const vtkSMViewProxy&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
