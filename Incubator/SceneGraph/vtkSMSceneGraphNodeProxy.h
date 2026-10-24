// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMSceneGraphNodeProxy
 * @brief   a node in a ParaView scenegraph
 *
 * A scenegraph node is deliberately empty of behavior. It exists to give the
 * node a type, so the manager's API and Python wrapping can take and return
 * `vtkSMSceneGraphNodeProxy*` instead of opaque `vtkSMProxy*`. Every
 * scenegraph algorithm (composition, traversal, apply, etc) lives in the
 * manager class (`vtkSMSceneGraphProxy`). Node state lives in the proxy's
 * properties (declared in `Plugins/SceneGraph/SceneGraph_SM.xml`) and is
 * read/written with vtkSMPropertyHelper.
 *
 * A node has no backing VTK object, and none of its properties include a
 * `command`. Because the manager reads and writes node state on the
 * client, and the generic proxy machinery saves it to state, no properties
 * that live on the node need to be pushed anywhere. Instead, the manager
 * decides which, if any, Representation properties to push based on a
 * client-side traversal of the graph.
 *
 * @sa vtkSMSceneGraphProxy
 */
#ifndef vtkSMSceneGraphNodeProxy_h
#define vtkSMSceneGraphNodeProxy_h

#include "vtkPVIncubatorSceneGraphModule.h" // for export macro
#include "vtkSMProxy.h"

class VTKPVINCUBATORSCENEGRAPH_EXPORT vtkSMSceneGraphNodeProxy : public vtkSMProxy
{
public:
  static vtkSMSceneGraphNodeProxy* New();
  vtkTypeMacro(vtkSMSceneGraphNodeProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMSceneGraphNodeProxy();
  ~vtkSMSceneGraphNodeProxy() override;

private:
  vtkSMSceneGraphNodeProxy(const vtkSMSceneGraphNodeProxy&) = delete;
  void operator=(const vtkSMSceneGraphNodeProxy&) = delete;
};

#endif
