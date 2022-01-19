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

#include "vtkPacket.h" // for vtkPacket
#include "vtkProvider.h"
#include "vtkServerManagerCoreModule.h"

class vtkObjectStore;
class vtkMultiProcessController;
class vtkProxyDefinitionManager;

class VTKSERVERMANAGERCORE_EXPORT vtkRemoteObjectProvider : public vtkProvider
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
  vtkSmartPointer<vtkObjectStore> GetObjectStore() const;
  ///@}

  ///@{
  /**
   * Get/Set the controller to use to reduce information in a distributed
   * environment.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkMultiProcessController* GetController() const;
  ///@}

  ///@{
  /**
   * Get/Set the proxy definition manager.
   */
  void SetProxyDefinitionManager(vtkProxyDefinitionManager* mgr);
  vtkProxyDefinitionManager* GetProxyDefinitionManager() const;
  ///@}

  ///@{
  /**
   * These methods can be used to construct a message to send to provider.
   */
  static vtkPacket UpdateState(vtkTypeUInt32 gid, const vtkJson& json);
  static vtkPacket UpdateInformation(vtkTypeUInt32 gid);
  static vtkPacket UpdatePipeline(vtkTypeUInt32 gid, double time);
  ///@}

  ///@{
  static const std::string CHANNEL_NOTIFY_UPDATE() { return "notify::update"; }
  static std::vector<vtkTypeUInt32> ParseNotifyUpdate(const vtkPacket& packet);
  ///@}
protected:
  vtkRemoteObjectProvider();
  ~vtkRemoteObjectProvider() override;

  void InitializeInternal(vtkService* service) override;

private:
  vtkRemoteObjectProvider(const vtkRemoteObjectProvider&) = delete;
  void operator=(const vtkRemoteObjectProvider&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
