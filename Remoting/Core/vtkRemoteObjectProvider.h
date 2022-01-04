/*=========================================================================

  Program:   ParaView
  Module:    vtkRemoteObjectProvider.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkRemoteObjectProvider
 * @brief
 *
 */

#ifndef vtkRemoteObjectProvider_h
#define vtkRemoteObjectProvider_h

#include "vtkProvider.h"
#include "vtkRemotingCoreModule.h"

class vtkObjectStore;
class vtkProxyDefinitionManager;

class VTKREMOTINGCORE_EXPORT vtkRemoteObjectProvider : public vtkProvider
{
public:
  static vtkRemoteObjectProvider* New();
  vtkTypeMacro(vtkRemoteObjectProvider, vtkProvider);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the object store to use.
   */
  void SetObjectStore(vtkObjectStore* store);
  vtkObjectStore* GetObjectStore() const;
  ///@}

  ///@{
  /**
   * Get/Set the proxy definition manager.
   */
  void SetProxyDefinitionManager(vtkProxyDefinitionManager* mgr);
  vtkProxyDefinitionManager* GetProxyDefinitionManager() const;
  ///@}

protected:
  vtkRemoteObjectProvider();
  ~vtkRemoteObjectProvider();

  void InitializeInternal(vtkService* service) override;

private:
  vtkRemoteObjectProvider(const vtkRemoteObjectProvider&) = delete;
  void operator=(const vtkRemoteObjectProvider&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
