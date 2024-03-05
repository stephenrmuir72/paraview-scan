// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMBivariateTextureRepresentationProxy
 * @brief   TODO
 *
 */

#ifndef vtkSMBivariateTextureRepresentationProxy_h
#define vtkSMBivariateTextureRepresentationProxy_h

#include "vtkBivariateRepresentationsModule.h" // for export macro
#include "vtkSMRepresentationProxy.h"

class vtkPVArrayInformation;

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkSMBivariateTextureRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMBivariateTextureRepresentationProxy* New();
  vtkTypeMacro(vtkSMBivariateTextureRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called after the view updates.
   */
  void ViewUpdated(vtkSMProxy* view) override;

protected:
  vtkSMBivariateTextureRepresentationProxy();
  ~vtkSMBivariateTextureRepresentationProxy() override;

  /**
   * Test
   */
  void CreateVTKObjects() override;

  // Test
  void OnPropertyUpdated(vtkObject*, unsigned long, void* calldata);

private:
  vtkSMBivariateTextureRepresentationProxy(const vtkSMBivariateTextureRepresentationProxy&) = delete;
  void operator=(const vtkSMBivariateTextureRepresentationProxy&) = delete;

  vtkSMProxy* CurrentView = nullptr;
};

#endif
