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
#include "vtkRemotingServerManagerModule.h" // for exports
#include "vtkSMProxyDefinitionsRange.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

class vtkPVXMLElement;

#include <memory> // for std::unique_ptr
#include <string> // for std::string
#include <vtk_pugixml.h>

class VTKREMOTINGSERVERMANAGER_EXPORT vtkProxyDefinitionManager : public vtkObject
{
public:
  static vtkProxyDefinitionManager* New();
  vtkTypeMacro(vtkProxyDefinitionManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns a proxy definition, if found. If not found, returns nullptr.
   * This does not raise any errors if the proxy definition is not found.
   */
  std::shared_ptr<pugi::xml_document> FindProxy(
    const std::string& group, const std::string& name) const;

  /**
   * Temporary API that returns a vtkPVXMLElement instead.
   */
  vtkSmartPointer<vtkPVXMLElement> FindProxyLegacy(
    const std::string& group, const std::string& name) const;

  /**
   * Load configuration XML
   */
  bool LoadConfigurationXML(const pugi::xml_node& root);
  bool LoadConfigurationXML(const std::string& xmlcontents);

  enum Events
  {
    ProxyDefinitionsUpdated = 2000,
  };

  vtkSMProxyDefinitionsRange GetDefinitions(const std::string& groupName = "");

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
