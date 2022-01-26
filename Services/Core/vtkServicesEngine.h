/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkServicesEngine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkServicesEngine
 * @brief represents an engine capable of launching and connecting to services.
 *
 * vtkServicesEngine represents a service engine.
 */

#ifndef vtkServicesEngine_h
#define vtkServicesEngine_h

#include "vtkEventual.h" // for vtkEventual
#include "vtkObject.h"
#include "vtkServicesCoreModule.h" // for exports

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkService;
class vtkServiceEndpoint;
class vtkMultiProcessController;

class VTKSERVICESCORE_EXPORT vtkServicesEngine : public vtkObject
{
public:
  vtkTypeMacro(vtkServicesEngine, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkServicesEngine(const vtkServicesEngine&) = delete;
  vtkServicesEngine& operator=(const vtkServicesEngine&) = delete;

  /**
   * vtkServicesEngine is an abstract class. However, one can use `New` to instantiate
   * a concrete subclass using the vtkObjectFactory.
   */
  static vtkServicesEngine* New();

  ///@{
  /**
   * Get/Set the controller to use. This must be set before initialization.
   * By default, `vtkDummyController` will be created and used.
   *
   * This is the controller that will be used on an internal RPC thread.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkMultiProcessController* GetController() const;
  ///@}

  /**
   * Must be called once to initialize the ServicesEngine. Implementation specific
   * subclasses may use this to initialize any implementation specific
   * infrastructure.
   *
   * This creates an internal `rxcpp::run_loop` that is used to dispatch all
   * responses.
   *
   * Not thread safe. Must be called only on the owner thread.
   */
  void Initialize(const std::string& url);

  /**
   * `Initialize` overload that enables the ServicesEngine to us an existing run loop.
   */
  void Initialize(
    const std::string& url, const std::shared_ptr<rxcpp::schedulers::run_loop>& runLoop);

  /**
   * `Initialize` overload that enables using an existing
   * `rxcpp::observe_on_one_worker` coordination type to use to dispatch all
   * responses.
   */
  void Initialize(const std::string& url, rxcpp::observe_on_one_worker cordination);

  /**
   * Inverse of `Initialize`, intended to cleanup once the ServicesEngine is done.
   *
   * Not thread safe. Must be called only on the owner thread.
   */
  void Finalize();

  /**
   * Returns the URL for this ServicesEngine. Note, this may not be the same as the one
   * passed in to `Initialize`. Implementations may qualify the URL further as
   * appropriate.
   *
   * This method is thread-safe.
   */
  const std::string& GetUrl() const;

  /**
   * Creates a service with the given name. The service name must be unique.
   * Multiple services with the same name on a ServicesEngine are not supported.
   *
   * Not thread safe. Must be called only on the owner thread.
   */
  vtkSmartPointer<vtkService> CreateService(const std::string& serviceName);

  /**
   * Returns an existing service.
   *
   * This is thread safe.
   */
  vtkSmartPointer<vtkService> GetService(const std::string& serviceName) const;

  /**
   * Creates a service-endpoint. A service-endpoint is used to connect to and
   * communicate with a service.
   *
   * Not thread safe. Must be called only on the owner thread.
   */
  vtkSmartPointer<vtkServiceEndpoint> CreateServiceEndpoint(
    const std::string& serviceName, const std::string& url = std::string());

  /**
   * Returns an existing service.
   *
   * This is thread safe.
   */
  vtkSmartPointer<vtkServiceEndpoint> GetServiceEndpoint(const std::string& serviceName) const;

  /**
   * Returns the scheduler that can be used to schedule events on the ServicesEngine's
   * scheduler.
   */
  rxcpp::observe_on_one_worker GetCoordination() const;

  /**
   * Helper method to process events.
   *
   * Returns true if something was processed.
   */
  bool ProcessEvents() const;

  /**
   * Helper method to process events until a time interval has passed.
   */
  template <typename Rep, typename Period>
  void ProcessEventsFor(const std::chrono::duration<Rep, Period>& duration);

  /**
   * Helper method to process events until a time interval has passed.
   */
  template <typename Rep, typename Period, typename Predicate>
  bool ProcessEventsFor(
    const std::chrono::duration<Rep, Period>& duration, Predicate stopProcessing);

  /**
   * This is used to create a new controller that is a duplicate of the main
   * controller.
   *
   * When using MPI, this will be a controller with a different handle.
   */
  vtkSmartPointer<vtkMultiProcessController> NewController() const;

protected:
  vtkServicesEngine();
  ~vtkServicesEngine() override;

  virtual std::string InitializeInternal(const std::string& url) = 0;
  virtual void FinalizeInternal() = 0;

private:
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#include "vtkServicesEngine.txx"
#endif
