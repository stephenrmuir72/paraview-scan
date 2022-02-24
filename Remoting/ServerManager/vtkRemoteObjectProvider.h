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
 * @brief provider that adds support for remote-objects
 *
 * vtkRemoteObjectProvider is the provider to add support for handling messages
 * sent by vtkSMProxy for creation / updating / deletion of remote VTK objects.
 */

#ifndef vtkRemoteObjectProvider_h
#define vtkRemoteObjectProvider_h

#include "vtkPacket.h" // for vtkPacket
#include "vtkProvider.h"
#include "vtkRemotingServerManagerModule.h" // for exports

class vtkObjectStore;
class vtkProxyDefinitionManager;
class vtkPVInformation;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkRemoteObjectProvider : public vtkProvider
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
   * Get/Set the proxy definition manager.
   */
  void SetProxyDefinitionManager(vtkProxyDefinitionManager* mgr);
  vtkProxyDefinitionManager* GetProxyDefinitionManager() const;
  ///@}

  ///@{
  /**
   * These methods can be used to construct a message to send to provider.
   */
  static vtkPacket UpdateState(vtkTypeUInt32 gid, const vtkNJson& json);
  static vtkPacket UpdateInformation(vtkTypeUInt32 gid);
  static vtkPacket UpdatePipeline(vtkTypeUInt32 gid, double time);
  static vtkPacket DeleteObject(vtkTypeUInt32 gid);
  static vtkPacket InvokeCommand(vtkTypeUInt32 gid, const std::string& command);
  static vtkPacket GatherInformation(vtkPVInformation* information, vtkTypeUInt32 targetGID);
  ///@}

  ///@{
  static const std::string CHANNEL_NOTIFY_UPDATE() { return "notify::update"; }
  static std::vector<vtkTypeUInt32> ParseNotifyUpdate(const vtkPacket& packet);
  ///@}

  ///@{
  struct vtkProgressItem
  {
    std::string Message;
    int8_t Progress;
    vtkTypeUInt32 GlobalID;
    bool operator==(const vtkProgressItem& other) const
    {
      return this->Message == other.Message && this->Progress == other.Progress &&
        this->GlobalID == other.GlobalID;
    }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(vtkProgressItem, Message, Progress, GlobalID);
  };
  static const std::string CHANNEL_PROGRESS() { return "pipeline::progress"; }
  static vtkProgressItem ParseProgress(const vtkPacket& packet);
  ///@}

  ///@{
  /**
   * Parse GatherInformation response.
   */
  static bool ParseResponse(const vtkPacket& packet, vtkPVInformation* information);
  static bool ParseResponse(const vtkNJson& json, vtkPVInformation* information);
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
