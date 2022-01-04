/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkServiceEndpoint.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkServiceEndpoint
 * @brief
 *
 */

#ifndef vtkServiceEndpoint_h
#define vtkServiceEndpoint_h

#include "vtkEventual.h" // for vtkEventual
#include "vtkObject.h"
#include "vtkServicesCoreModule.h" // for exports

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkChannelSubscription;
class vtkPacket;
class vtkServicesEngine;

using vtkPacketObservable = rxcpp::observable<vtkPacket>;
class VTKSERVICESCORE_EXPORT vtkServiceEndpoint : public vtkObject
{
public:
  vtkTypeMacro(vtkServiceEndpoint, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkServiceEndpoint(const vtkServiceEndpoint&) = delete;
  vtkServiceEndpoint& operator=(const vtkServiceEndpoint&) = delete;

  /**
   * Connect to a service.
   *
   * This is not thread safe and must only be called on the owner thread.
   */
  vtkEventual<bool> Connect();

  /**
   * Shutdown the service endpoint.
   *
   * This is not thread safe and must only be called on the owner thread.
   */
  void Shutdown();

  /**
   * Returns the name for the service this endpoint is connected to.
   */
  const std::string& GetServiceName() const;

  /**
   * Returns the url for the service this endpoint is connected to.
   */
  const std::string& GetServiceUrl() const;

  /**
   * Provides access to the engine.
   */
  vtkServicesEngine* GetEngine() const;

  /**
   * Send a message (no reply expected).
   *
   * This method can only be called after`Connect`. Note calling `Connect` does
   * not mean that the connect to the target service
   * has been established when the function returns. This method, however, can
   * be called irrespective of whether the connection has been fully
   * established. In that case, this method may block till that time.
   *
   * Calling this method before `Connect` raises an error. Calling it after the
   * endpoint has been shutdown is quietly ignored.
   *
   * This method is thread safe.
   */
  void SendMessage(const vtkPacket& packet) const;

  /**
   * Send a message with a reply.
   *
   * This method can only be called after`Connect`. Note calling `Connect` does
   * not mean that the connect to the target service
   * has been established when the function returns. This method, however, can
   * be called irrespective of whether the connection has been fully
   * established. In that case, this method may block till that time.
   *
   * Calling this method before `Connect` raises an error. Calling it after the
   * endpoint has been shutdown is quietly ignored.
   *
   * This method is thread safe.
   */
  vtkEventual<vtkPacket> SendRequest(const vtkPacket& packet) const;

  ///@{
  /**
   * Subscribe to a channel. This will return a new vtkChannelSubscription
   * instance each time this method is called. However, the actual request to
   * subscribe to the channel is only sent to the service the first time.
   *
   * This method is thread safe.
   */
  vtkSmartPointer<vtkChannelSubscription> Subscribe(const std::string& channel);
  vtkSmartPointer<vtkChannelSubscription> Subscribe(
    const std::string& channel, const rxcpp::observe_on_one_worker& coordination);
  ///@}

protected:
  vtkServiceEndpoint();
  ~vtkServiceEndpoint() override;

  virtual void InitializeInternal() = 0;
  virtual vtkEventual<bool> ConnectInternal(
    const std::string& serviceName, const std::string& url) = 0;
  virtual void ShutdownInternal() = 0;
  virtual void SendMessageInternal(const vtkPacket& packet) const = 0;
  virtual vtkEventual<vtkPacket> SendRequestInternal(const vtkPacket& packet) const = 0;
  virtual vtkEventual<bool> SubscribeInternal(const std::string& channel) = 0;
  virtual void UnsubscribeInternal(const std::string& channel) = 0;

  /**
   * Subclasses can use this method to post a message received on a channel.
   *
   * This method is thread safe.
   */
  void DispatchOnChannel(const std::string& channel, const vtkPacket& packet, bool cached) const;

  ///@{
  friend class vtkChannelSubscription;
  void Unsubscribe(const std::string& channel);
  ///@}
private:
  ///@{
  /**
   * API accessible only to vtkServicesEngine.
   */
  friend class vtkServicesEngine;
  static vtkServiceEndpoint* New();
  void Initialize(
    vtkServicesEngine* engine, const std::string& serviceName, const std::string& sessionUrl);
  void UnsetEngine();
  ///@}

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
