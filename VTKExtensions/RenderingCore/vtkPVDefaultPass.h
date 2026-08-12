// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVDefaultPass
 * @brief   encapsulates the traditional OpenGL pipeline
 * (minus the camera).
 *
 * vtkPVDefaultPass is a simple render pass that encapsulates the traditional
 * OpenGL pipeline (minus the camera).
 */

#ifndef vtkPVDefaultPass_h
#define vtkPVDefaultPass_h

#include "vtkPVVTKExtensionsRenderingCoreModule.h" // for export macro/
#include "vtkRenderPass.h"
#include "vtkWrappingHints.h" // for VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKPVVTKEXTENSIONSRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkPVDefaultPass : public vtkRenderPass
{
public:
  static vtkPVDefaultPass* New();
  vtkTypeMacro(vtkPVDefaultPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual rendering code.
   */
  void Render(const vtkRenderState* render_state) override;

protected:
  vtkPVDefaultPass();
  ~vtkPVDefaultPass() override;

private:
  vtkPVDefaultPass(const vtkPVDefaultPass&) = delete;
  void operator=(const vtkPVDefaultPass&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
