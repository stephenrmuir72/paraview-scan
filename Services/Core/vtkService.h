/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkService.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkService
 * @brief represents a service
 *
 * vtkService represents a service. A service is a entity that awaits requests /
 * messages from a vtkServiceEndpoint and responds to them. vtkService also
 * supports a "publish" mechanism, which allows a service to send messages via a
 * stream.
 */

#ifndef vtkService_h
#define vtkService_h

#include "vtkEventual.h" // for vtkEventual
#include "vtkObject.h"
#include "vtkPacket.h"             // for vtkPacket.
#include "vtkServicesCoreModule.h" // for exports
#include "vtkSmartPointer.h"       // for vtkSmartPointer

#include <memory> // for std::unique_ptr

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkChannelSubscription;
class vtkMultiProcessController;
class vtkPacket;
class vtkServiceReceiver;
class vtkServicesEngine;

class VTKSERVICESCORE_EXPORT vtkService : public vtkObject
{
public:
  vtkTypeMacro(vtkService, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkService(const vtkService&) = delete;
  vtkService& operator=(const vtkService&) = delete;

  /**
   * Start the service.
   *
   * Not thread-safe. Must be called only on the owner thread.
   */
  bool Start();

  /**
   * Call to shutdown the service.
   *
   * Not thread-safe. Must be called only on the owner thread.
   */
  void Shutdown();

  /**
   * Returns the name for the service.
   *
   * This method is thread-safe.
   */
  const std::string& GetName() const;

  /**
   * Returns an observable for the service's incoming message stream.
   *
   * When setting up a service, one typically wants to setup observers that
   * handle requests and respond to messages. This is done this observable.
   *
   * It is acceptable to use this method to setup the message processing pipeline
   * before connecting to a service.
   *
   * By default, the observable will be setup to observe on the service's run
   * loop.
   *
   * This method is thread-safe.
   */
  rxcpp::observable<vtkServiceReceiver> GetRequestObservable(bool skipEventLoop = false) const;

  /**
   * Enable caching on a channel. Any subsequent messages on this channel will
   * get cached i.e. any subscriber that subscribes will receive the latest
   * message published on the channel irrespective of when it was published.
   *
   * Note, since this preserve the latest packet internally, one must call
   * `ClearChannelCache` to release the cached packet.
   */
  void EnableChannelCache(const std::string& channelName) const;

  /**
   * Deletes a previously created channel. While not strictly necessary, helps
   * cleanup any cached packets for that channel and makes the name available
   * for reuse.
   *
   * This method is thread-safe.
   */
  void ClearChannelCache(const std::string& channelName) const;

  /**
   * Publish a message on a named channel.
   *
   * This method is thread-safe.
   */
  void Publish(const std::string& channelName, const vtkPacket& packet) const;

  /**
   * Subscribe to a channel on another service. This is a mechanism for services
   * to send packets between one-another.
   *
   * This method is thread-safe.
   */
  vtkSmartPointer<vtkChannelSubscription> Subscribe(
    const std::string& channel, const std::string& serviceName) const;

  /**
   * Provides access to the run-loop to this service. This can be used with
   * rxcpp operators as follows:
   *
   * @code{cpp}
   *
   * <observable>
   *  .observe_on(rxcpp::observe_on_run_loop(service->GetRunLoop()))
   *  .subscribe(....)
   *
   * @endcode
   *
   * This method is thread-safe.
   */
  virtual const rxcpp::schedulers::run_loop& GetRunLoop() const = 0;

  /**
   * Convenience method to provide rxcpp coordination type that can be passed
   * to rxcpp::observe_on(...)
   *
   * @code{cpp}
   *
   * <observable>
   *  .observe_on(server->ObserveOnRunLoop())
   *  .subscribe(....)
   *
   * @endcode
   *
   * This method is thread-safe.
   */
  auto GetRunLoopScheduler() const { return rxcpp::observe_on_run_loop(this->GetRunLoop()); }

  /**
   * Provides access to the engine that this service belongs to.
   *
   * This method is thread-safe.
   */
  vtkServicesEngine* GetEngine() const;

  ///@{
  /**
   * Get the controller to use. The controller is provided by the
   * vtkServicesEngine during initialization. Each service gets its own
   * controller to avoid interfering with one another. If vtkServicesEngine
   * doesn't have a MPI-controller, then the services too inherit a
   * dummy-controller.
   *
   * Note, the controller is primarily intended to be used in the service's
   * main thread.
   */
  vtkMultiProcessController* GetController() const;
  ///@}

protected:
  vtkService();
  ~vtkService() override;

  virtual void InitializeInternal() = 0;
  virtual bool StartInternal() = 0;
  virtual void ShutdownInternal() = 0;

  /**
   * Subclasses can use this method to dispatch a message / request received by
   * the service.
   *
   * This method is thread safe.
   */
  void Dispatch(vtkServiceReceiver&& message);

  /**
   * Returns an observable for the given channel.
   */
  rxcpp::observable<std::tuple<vtkPacket, bool>> GetChannelObservable(
    const std::string& channelName);

private:
  ///@{
  /**
   * API accessible only to vtkServicesEngine.
   */
  friend class vtkServicesEngine;
  static vtkService* New();
  void Initialize(vtkServicesEngine* engine, const std::string& name);
  void UnsetEngine();
  ///@}

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  static void RMIDispatch(
    void* localArg, void* remoteArg, int remoteArgLength, int remoteProcessId);
};

class VTKSERVICESCORE_EXPORT vtkServiceReceiver
{
  vtkSmartPointer<vtkService> Service;
  mutable vtkEventual<vtkJson> Response;
  vtkPacket Packet;

public:
  vtkServiceReceiver(
    vtkService* service, const vtkPacket& packet, const vtkEventual<vtkJson>& response);
  vtkServiceReceiver(const vtkServiceReceiver&) = default;
  vtkServiceReceiver& operator=(const vtkServiceReceiver&) = default;
  ~vtkServiceReceiver() = default;

  /**
   * Send a response back.
   */
  void Respond(const vtkJson& packet) const;
  void Respond(const vtkPacket& packet) const;

  /**
   * Provides access to the packet received.
   */
  const vtkPacket& GetPacket() const { return this->Packet; }

  /**
   * Publish on a named channel.
   */
  void Publish(const std::string& channel, const vtkPacket& packet) const;
};

#endif
