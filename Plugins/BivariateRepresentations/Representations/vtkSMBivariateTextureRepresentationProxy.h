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

#include <list>

class vtkPVArrayInformation;
class vtkSMLink;

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
   * Called each time the internal representation of the composite representation is 
   * changed (the bivariate texture representation being one of them).
   * In addition to instanciate the VTK objects if needed, takes care to unregister
   * the custom textured scalar bar proxy to avoid other internal representations
   * to use it when switching back to them.
   */
  void CreateVTKObjects() override;

  // Test
  void OnPropertyUpdated(vtkObject*, unsigned long, void* calldata);
  void Test(vtkObject*, unsigned long, void* calldata);

private:
  vtkSMBivariateTextureRepresentationProxy(const vtkSMBivariateTextureRepresentationProxy&) = delete;
  void operator=(const vtkSMBivariateTextureRepresentationProxy&) = delete;

  vtkSMProxy* CurrentView = nullptr;

  /**
   * Setup all links between properties and informations. It also update properties
   * information before anything else. Usually called in CreateVTKObject() function.
   */
  void SetupPropertiesLinks();
  // vtkSMProxy* FindScalarBarRepresentation();

  vtkSMProxy* TexturedScalarBarProxy = nullptr;
  bool Initialized = true;

  typedef std::list<vtkSmartPointer<vtkSMLink>> LinksType;
  LinksType Links;
};

#endif
