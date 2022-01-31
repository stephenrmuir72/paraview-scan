/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMProxyManager
 * @brief manager to create and manage proxies
 *
 * vtkSMProxyManager is used to create and manage proxies on a vtkClientSession.
 * Proxies are used to create and control vtkObject subclasses on a "remote"
 * vtkServerSession that the vtkClientSession is connected to.
 */

#ifndef vtkSMProxyManager_h
#define vtkSMProxyManager_h

#include "vtkServerManagerCoreModule.h" // for exports

#include "vtkObject.h"
#include "vtkSMPrototypeProxiesRange.h"  // for vtkSMPrototypeProxiesRange
#include "vtkSMRegisteredProxiesRange.h" // for vtkSMRegisteredProxiesRange
#include "vtkSmartPointer.h"             // for vtkSmartPointer

class vtkSMProxy;
class vtkClientSession;

class VTKSERVERMANAGERCORE_EXPORT vtkSMProxyManager : public vtkObject
{
public:
  static vtkSMProxyManager* New();
  vtkTypeMacro(vtkSMProxyManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the vtkClientSession that this proxy manager is for. Each
   * vtkClientSession has exactly one vtkSMProxyManager.
   */
  void SetSession(vtkClientSession* session);
  vtkClientSession* GetSession() const;
  ///@}

  /**
   * Create a new proxy given the type.
   */
  vtkSmartPointer<vtkSMProxy> NewProxy(const std::string& group, const std::string& name);

  /**
   * Register a proxy.
   */
  void RegisterProxy(
    const std::string& group, const std::string& name, const vtkSmartPointer<vtkSMProxy>& proxy);

  /**
   * Register a proxy assigning it a unique name automatically. The proxy name
   * is returned.
   */
  std::string RegisterProxy(const std::string& group, const vtkSmartPointer<vtkSMProxy>& proxy);

  /**
   * Unregister a proxy.
   */
  void UnRegisterProxy(
    const std::string& group, const std::string& name, const vtkSmartPointer<vtkSMProxy>& proxy);

  /**
   * Unregister all proxies registered under the specified group with specified
   * name.
   */
  void UnRegisterProxies(const std::string& group, const std::string& name);

  /**
   * Unregister the proxy from all groups and names.
   */
  void UnRegisterProxy(const vtkSmartPointer<vtkSMProxy>& proxy);

  /**
   * Given a group, returns a name not already used for proxies registered in
   * the given group. The prefix is used to come up with a new name.
   * if alwaysAppend is true, then a suffix will always be appended, if not,
   * the prefix may be used directly if possible.
   */
  std::string GetUniqueProxyName(
    const std::string& group, const std::string& prefix, bool alwaysAppend = true) const;

  /**
   * Returns a proxy with the given global id, if one exists, otherwise returns
   * nullptr.
   */
  vtkSmartPointer<vtkSMProxy> FindProxy(vtkTypeUInt32 gid) const;

  vtkSMRegisteredProxiesRange GetRegisteredProxies(const std::string& groupName = "");
  vtkSMPrototypeProxiesRange GetPrototypeProxies();

protected:
  vtkSMProxyManager();
  ~vtkSMProxyManager() override;

private:
  vtkSMProxyManager(const vtkSMProxyManager&) = delete;
  void operator=(const vtkSMProxyManager&) = delete;

  void HandleProxyEvent(vtkObject* sender, unsigned long eventId, void* callData);
  void MarkPipelineUpdated(vtkSMProxy* proxy);
  class vtkInternals;
  friend class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
