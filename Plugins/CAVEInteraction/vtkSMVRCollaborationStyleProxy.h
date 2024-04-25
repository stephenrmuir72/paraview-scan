// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRCollaborationStyleProxy_h
#define vtkSMVRCollaborationStyleProxy_h

#include "vtkSMVRInteractorStyleProxy.h"
#include "vtkVRQueue.h"

#include <memory> // for std::unique_ptr

class QString;

class vtkSMVRCollaborationStyleProxy : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRCollaborationStyleProxy* New();
  vtkTypeMacro(vtkSMVRCollaborationStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /*
   * Set event names used to associate tracker events with
   * avatar body parts. This class will only generate Move3D
   * events and invoke them when events have the names
   * selected here.
   */
  void SetHeadEventName(QString* eventName);
  void SetLeftHandEventName(QString* eventName);
  void SetRightHandEventName(QString* eventName);

protected:
  vtkSMVRCollaborationStyleProxy();
  ~vtkSMVRCollaborationStyleProxy() override;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRCollaborationStyleProxy(
    const vtkSMVRCollaborationStyleProxy&) = delete;              // Not implemented.
  void operator=(const vtkSMVRCollaborationStyleProxy&) = delete; // Not implemented.

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internal;
};

#endif // vtkSMVRCollaborationStyleProxy_h
