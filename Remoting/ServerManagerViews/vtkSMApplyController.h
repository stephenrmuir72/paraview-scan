/*=========================================================================

  Program:   ParaView
  Module:    vtkSMApplyController.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMApplyController
 * @brief
 *
 */

#ifndef vtkSMApplyController_h
#define vtkSMApplyController_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerViewsModule.h" // for exports

#include <memory> // for std::unique_ptr

class vtkSMProxy;
class vtkSMSessionProxyManager;
class vtkSMSourceProxy;
class vtkSMViewProxy;

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkSMApplyController : public vtkObject
{
public:
  static vtkSMApplyController* New();
  vtkTypeMacro(vtkSMApplyController, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Use this method to tag a new source proxy so that on create new
   * representation to show the data on apply in preferred view.
   */
  static void MarkShowOnApply(vtkSMProxy* proxy);
  static void UnmarkShowOnApply(vtkSMProxy* proxy);
  static bool IsMarkedShowOnApply(vtkSMProxy* proxy);
  ///@}

  /**
   * Execute the apply logic for a particular session.
   */
  virtual void Apply(vtkSMSessionProxyManager* pxm);

protected:
  vtkSMApplyController();
  ~vtkSMApplyController();

  vtkSMViewProxy* GetActiveView(vtkSMSessionProxyManager* pxm) const;

  virtual void HideInputs(vtkSMProxy* producer, vtkSMViewProxy* view);

  virtual void ResetCamera(vtkSMViewProxy* view);

  virtual void Show(vtkSMSourceProxy* producer);

private:
  vtkSMApplyController(const vtkSMApplyController&) = delete;
  void operator=(const vtkSMApplyController&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
