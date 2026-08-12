// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVRendererCuller
 * @brief Culls props in the composited renderer when the local process is not participating in
 * compositing.
 *
 * In multi-process rendering modes, data delivered to a set of ranks is not cleared until the data
 * pipeline updates. This avoids having to redeliver data when simply switching between remote and
 * local rendering modes. However, this can cause issues: when rendering a Sphere in client-server
 * mode with remote rendering disabled, the geometry is delivered to the client. When switching to
 * force remote-rendering, the geometry previously delivered to the client is still there and will
 * get rendered, even though it will be replaced with pixels from the server. This culler enables
 * culling of props in the composited renderer on the local process when it's not participating in
 * the compositing.
 *
 * The culler also supports a DoNotCullList to exclude specific props from culling, such as
 * vtkPVGridAxes3DActor which is an exception to the standard culling rule.
 *
 * @see vtkCuller
 * @see vtkPVGridAxes3DActor
 */

#ifndef vtkPVRenderCuller_h
#define vtkPVRenderCuller_h

#include "vtkCuller.h"
#include "vtkPVVTKExtensionsRenderingCoreModule.h" // for export macro
#include "vtkWrappingHints.h"                      // For VTK_MARSHALAUTO

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKPVVTKEXTENSIONSRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkPVRendererCuller : public vtkCuller
{
public:
  static vtkPVRendererCuller* New();
  vtkTypeMacro(vtkPVRendererCuller, vtkCuller);

  double Cull(
    vtkRenderer* vtkNotUsed(ren), vtkProp** propList, int& listLength, int& initialized) override;

  vtkSetMacro(RenderOnLocalProcess, bool);
  vtkGetMacro(RenderOnLocalProcess, bool);

  void AddToDoNotCullList(vtkProp* prop);

  void RemoveFromDoNotCullList(vtkProp* prop);

protected:
  vtkPVRendererCuller();
  ~vtkPVRendererCuller() override;
  bool RenderOnLocalProcess = false;

private:
  vtkPVRendererCuller(const vtkPVRendererCuller&) = delete;
  void operator=(const vtkPVRendererCuller&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
