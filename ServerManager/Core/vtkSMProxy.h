/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMProxy
 * @brief a proxy for a remote vtkObject (or subclass)
 *
 * vtkSMProxy is one of the core building blocks of ParaView. It enables
 * creation, deletion and manipulation of a VTK-object on services connected to
 * to by a `vtkClientSession`.
 *
 * In VTK, one typically creates data processing pipelines using sources, filters,
 * and other algorithms. We collectively refer to all such objects as VTK-objects.
 * In ParaView, proxies help create such data processing and rendering pipelines
 * on remote and/or concurrent services.
 *
 * @section CreatingAProxy Creating a proxy
 *
 * To create a proxy, one must use the vtkSMProxyManager. Each vtkClientSession
 * sets up a vtkSMProxyManager during initialization. The ProxyManager
 * configures itself will all proxy types available on the connected services
 * when the connection is established.
 *
 * To create a proxy, use `vtkSMProxyManager::NewProxy`.
 *
 * @code{cpp}
 *
 * vtkSMProxyManager* pxm = ...
 * vtkSmartPointer<vtkSMProxy> proxy = pxm->NewProxy("<group>", "<name>");
 *
 * @endcode
 *
 * All proxy definitions are identified by name and grouped under a named proxy group.
 * The group name helps avoid name conflicts and also classify definitions for
 * applications.
 *
 * During creation the proxy is assigned a unique id called `GlobalID`. This id
 * is unique for the specific client session. Thus proxies on two different
 * sessions may have conflicting global ids.
 *
 * @section Events Events
 *
 * * vtkSMCommand::ConsumersChangedEvent: This event is fired anytime the
 * consumers for this proxy change. The consumers are other proxies that depend
 * on this proxy i.e. this proxy has been set (or unset) as the value for either a
 * vtkSMProxyProperty or a vtkSMInputProperty on the other proxy. `callData`
 * passed to the event handler is nullptr.
 */

#ifndef vtkSMProxy_h
#define vtkSMProxy_h

#include "vtkJson.h" // for vtkJson
#include "vtkObject.h"
#include "vtkServerManagerCoreModule.h" // for exports

#include <vtk_pugixml.h> // for pugi

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkClientSession;
class vtkSMOutputPort;
class vtkSMProxyManager;
class vtkSMProperty;
class vtkSMStateContext;

class VTKSERVERMANAGERCORE_EXPORT vtkSMProxy : public vtkObject
{
public:
  static vtkSMProxy* New();
  vtkTypeMacro(vtkSMProxy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the unique id assigned to this proxy.
   */
  vtkTypeUInt32 GetGlobalID() const;

  /**
   * Returns the location for the objects this proxy represents.
   */
  vtkTypeUInt32 GetLocation() const;

  /**
   * Returns the session this proxy belongs to.
   */
  vtkClientSession* GetSession() const;

  /**
   * Returns the proxy manager for the session this proxy belongs to.
   */
  vtkSMProxyManager* GetProxyManager() const;

  /**
   * Get the proxy name.
   */
  const std::string& GetName() const;

  /**
   * Get proxy group.
   */
  const std::string& GetGroup() const;

  /**
   * Get label. If not defined in definition, one will be created based on the
   * proxy's Name.
   */
  std::string GetXMLLabel() const;

  /**
   * Push state from the proxy to the VTK object corresponding to this proxy on
   * the target services.
   *
   * Returns true if state updates were pushed. State updates are not pushed if
   * none of the properties (or properties on subproxies) were changed since the
   * last call to UpdateVTKObjects.
   */
  bool UpdateVTKObjects();

  /**
   * Fetch properties. The observable will return true if the any properties
   * were changed otherwise returns false.
   *
   * This method intentionally returns an observable instead of a vtkEventual.
   * If this returned an vtkEventual, the calling code could call
   * `Wait` which can cause a deadlock since `UpdateInformation` enqueue code
   * that needs to execute on the main thread
   */
  rxcpp::observable<bool> UpdateInformation();

  /**
   * Request a pipeline update. Only makes sense to call this on proxies for
   * `vtkAlgorithm` subclasses. On all others, this has will have no effect.
   *
   * The observable returns true when the update completes successfully.
   */
  rxcpp::observable<bool> UpdatePipeline(double time);

  /**
   * Invoke command.
   */
  void InvokeCommand(const std::string& name);

  /**
   * Returns the number of output ports, if any. A proxy may have one or more
   * output ports if it represents a vtkAlgorithm or subclass with one or more
   * outputs.
   */
  int GetNumberOfOutputPorts() const;

  /**
   * Returns the vtkSMOutputPort for a specific output port index, if any.
   */
  vtkSMOutputPort* GetOutputPort(int index) const;

  ///@{
  /**
   * vtkSMProxy and vtkSMOutputPort form a reference loop and hence we
   * need garbage collection.
   */
  void Register(vtkObjectBase* o) override { this->RegisterInternal(o, true); }
  void UnRegister(vtkObjectBase* o) override { this->UnRegisterInternal(o, true); }
  ///@}

  /**
   * Returns a property given its name. This can be a property of this proxy
   * or an exposed property from one of its subproxies. Returns nullptr, if none
   * is found.
   */
  vtkSMProperty* GetProperty(const std::string& name) const;

  //@{
  /**
   * Log name is a name for this proxy that will be used when logging status
   * messages. This helps make the log more user friendly by making sure it uses
   * names that the user can easily map to objects shown in the user interface.
   *
   * This method will set the log name for this proxy and iterate over all
   * subproxies and set log name for each using the provided `name` as prefix.
   *
   * Furthermore, for all properties with vtkSMProxyListDomain, it will set log
   * name for proxies in those domains to use the provided `name` as prefix as
   * well.
   *
   * @note The use of this name for any other purpose than logging is strictly
   * discouraged.
   */
  void SetLogName(const std::string& name);
  const std::string& GetLogName() const;
  //@}

  /**
   * A helper that makes up an default name if none is provided.
   */
  const std::string& GetLogNameOrDefault() const;

protected:
  vtkSMProxy();
  ~vtkSMProxy() override;

  /**
   * Read the XML and populate the proxy.
   */
  virtual bool ReadXMLAttributes(const pugi::xml_node& xml);

  /**
   * needed for garbage collection.
   */
  void ReportReferences(vtkGarbageCollector* collector) override;

  /**
   * Save proxy state. `verbose` is set to true to save extraneous information
   * useful for public facing state files. This will include things like domains
   * etc.
   *
   * It tstamp is non-zero, then only those properties that have been modified
   * since the tstamp are saved to the state.
   */
  vtkJson SaveState(bool verbose, vtkMTimeType tstamp = 0) const;

  /**
   * Load state. `context` is used to locate other proxies, for example, that
   * may be referenced in the state.
   */
  bool LoadState(const vtkJson& json, vtkSMStateContext* context);

  ///@{
  /**
   * These method is called before/after the first state for this proxy is pushed.
   * That indicates that the service-side VTK objects would potentially be
   * created.
   */
  virtual void PreCreateVTKObjects();
  virtual void PostCreateVTKObjects();
  ///@}

  ///@{
  /**
   * Add/Remove a consumer. The `proxy` is the proxy that depends on this proxy and
   * `property` identifies the property that establishes that dependency.
   *
   * We're making as assumption that the same proxy cannot be added to a
   * property twice. While that's not necessarily required, in practice that is
   * the case. We will need to change the implementation if that's no longer the
   * case.
   */
  void AddConsumer(vtkSMProxy* proxy, vtkSMProperty* property);
  void RemoveConsumer(vtkSMProxy* proxy, vtkSMProperty* property);
  friend class vtkSMPropertyProducer;
  ///@}

private:
  vtkSMProxy(const vtkSMProxy&) = delete;
  void operator=(const vtkSMProxy&) = delete;

  ///@{
  friend class vtkSMProxyManager;
  void SetSession(vtkClientSession* session);
  void SetGlobalID(vtkTypeUInt32 gid);
  void SetGroup(const std::string& group);
  void SetName(const std::string& name);

  /**
   * This method is called by vtkSMProxyManager when the service notify the
   * client that the VTK object has executed consequently indicating that
   * things like data-information may no longer be valid.
   */
  void MarkPipelineUpdated();
  ///@}

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  void HandlePropertyEvents(vtkObject* caller, unsigned long eventId, void* callData);

  /**
   * Internal method used by `SetLogName`
   */
  void SetLogNameInternal(
    const std::string& name, bool propagate_to_subproxies, bool propagate_to_proxylistdomains);
};

#endif
