/*=========================================================================

  Program:   ParaView
  Module:    vtkProxyDefinitionManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkProxyDefinitionManager
 * @brief manages proxy definitions
 *
 * vtkProxyDefinitionManager is thread-safe class that handles proxy
 * definitions.
 */

#ifndef vtkProxyDefinitionManager_h
#define vtkProxyDefinitionManager_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" // for exports

#include <memory> // for std::unique_ptr
class vtkPVXMLElement;

class VTKREMOTINGCORE_EXPORT vtkProxyDefinitionManager : public vtkObject
{
public:
  static vtkProxyDefinitionManager* New();
  vtkTypeMacro(vtkProxyDefinitionManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns a proxy definition, if found. If not found, returns nullptr.
   * This does not raise any errors if the proxy definition is not found.
   */
  vtkSmartPointer<vtkPVXMLElement> FindProxy(
    const std::string& group, const std::string& name) const;

  /**
   * Load configuration XML
   */
  bool LoadConfigurationXML(vtkPVXMLElement* root);
  bool LoadConfigurationXML(const char* xmlcontents);

  enum Events
  {
    ProxyDefinitionsUpdated = 2000,
  };

protected:
  vtkProxyDefinitionManager();
  ~vtkProxyDefinitionManager();

private:
  vtkProxyDefinitionManager(const vtkProxyDefinitionManager&) = delete;
  void operator=(const vtkProxyDefinitionManager&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
