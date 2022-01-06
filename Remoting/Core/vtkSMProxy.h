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
 * @brief
 *
 */

#ifndef vtkSMProxy_h
#define vtkSMProxy_h

#include "vtkJson.h" // for vtkJson
#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" // for exports

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

class VTKREMOTINGCORE_EXPORT vtkSMProxy : public vtkObject
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
   */
  rxcpp::observable<bool> UpdateInformation();

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
   */
  vtkJson SaveState(bool verbose) const;

  /**
   * Load state. `context` is used to locate other proxies, for example, that
   * may be referenced in the state.
   */
  bool LoadState(const vtkJson& json, vtkSMStateContext* context);

private:
  vtkSMProxy(const vtkSMProxy&) = delete;
  void operator=(const vtkSMProxy&) = delete;

  ///@{
  friend class vtkSMProxyManager;
  void SetSession(vtkClientSession* session);
  void SetGlobalID(vtkTypeUInt32 gid);
  void SetGroup(const std::string& group);
  void SetName(const std::string& name);
  ///@}

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  void HandlePropertyEvents(vtkObject* caller, unsigned long eventId, void* callData);
};

#endif
