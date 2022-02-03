/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSessionProxyManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMSessionProxyManager
 *  The vtkSMSessionProxyManager is esponsible for creating and
 *  managing proxies for a given session.
 *
 *
 * vtkSMSessionProxyManager is an instance that creates and manages proxies
 * for a given session/server.
 * It maintains a map of XML elements (populated by the XML parser) from
 * which it can create and initialize proxies and properties.
 * Once a proxy is created, it can either be managed by the user code or
 * the proxy manager. In the latter case, pass the control of the proxy to the
 * manager with RegisterProxy() and unregister it. At destruction, proxy
 * manager deletes all managed proxies.
 *
 * vtkSMSessionProxyManager is designed to work with only one session. When
 * the session on which it is attached closes, it has no role and should be
 * deleted right away.
 * @sa vtkProxyDefinitionManager
 *
 * @sa
 * Basic XML Proxy definition documentation:
 *
 * @sa
 * ------------- Proxy definition -------------
 * \code{.xml}
 *  <SourceProxy                   => Will create vtkSM + SourceProxy class.
 *         name="SphereSource"     => Key used to create the proxy.
 *         class="vtkSphereSource" => Concrete VTK class that does the real job.
 *         label="Sphere">         => Nice name used in menu and python shell.
 * \endcode
 *
 * @sa
 * ----------- Property definition -----------
 * \code{.xml}
 *    <DoubleVectorProperty        => Will create vtkSM + DoubleVectorProperty
 *                                    and vtkSI + DoubleVectorProperty class by
 *                                    default.
 *         name="Center"           => Name of the property:
 *                                    Example usage: vtkSMPropertyHelper(proxy, "Center").Set(0,1,2)
 *         command="SetCenter"     => Real method name that will be called on
 *                                    vtkObject when the property is updated.
 *         number_of_elements="3"  => Size of the vector.
 *         animateable="1"         => Tell the animation view that property
 *                                    can be used as an evolving property.
 *         default_values="0 0 0"> => The value that will be set at the
 *    </DoubleVectorProperty>         construction to the VTK object.
 *  </SourceProxy>
 * \endcode
 *
 * @sa
 * For custom behaviour the user can add some extra attributes:
 *
 *  - We can specify a custom SIProperty class to handle in a custom way the
 *    data on the server:
 * \code{.xml}
 *      <StringVectorProperty          => vtkSMStringVectorProperty class.
 *         name="ElementBlocksInfo"    => Property name.
 *         information_only="1"        => Can only be used to fetch data.
 *         si_class="vtkSISILProperty" => Class name to instantiate on the other side.
 *         subtree="Blocks"/>          => Extra attribute used by vtkSISILProperty.
 * \endcode
 *
 * @sa
 *  - We can trigger after any update a command to be executed:
 * \code{.xml}
 *      <Proxy name="LookupTable"
 *             class="vtkLookupTable"
 *             post_push="Build"       => The method Build() will be called each
 *                                        time a new property value is pushed to
 *                                        the VTK object.
 *             processes="dataserver|renderserver|client" >
 * </pre>
 *
 * @sa
 *  - We can force any property to push its value as soon as it is changed:
 * \code{.xml}
 *          <Property name="ResetFieldCriteria"
 *             command="ResetFieldCriteria"
 *             immediate_update="1">     => Modifying the property will result
 *                                          in an immediate push of it and the
 *                                          execution of the command on the vtkObject.
 * \endcode
 *
 * @sa
 *  - To show a source proxy or a filter inside the menu of ParaView we use a hint:
 * \code{.xml}
 *       <SourceProxy ...>
 *           <Hints>
 *              <ShowInMenu                  => The category attribute enables
 *                  category="PersoFilter"/>    specification of the sub-menu in which
 *                                              this proxy should be listed. (optional)
 *           </Hints>
 *       </SourceProxy>
 * \endcode
 */

#ifndef vtkSMSessionProxyManager_h
#define vtkSMSessionProxyManager_h

#include "vtkJsonFwd.h" // needed for vtkJson
#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" // needed for exports
#include "vtkSmartPointer.h"                // needed for vtkSmartPointer
#include "vtkWeakPointer.h"                 // needed for vtkWeakPointer

#include <set>    // needed for std::set
#include <string> // needed for std::string

class vtkClientSession;
class vtkCollection;
class vtkEventForwarderCommand;
class vtkProxyDefinitionManager;
class vtkPVXMLElement;
class vtkSMCompoundSourceProxy;
class vtkSMDocumentation;
class vtkSMLink;
class vtkSMProperty;
class vtkSMProxy;
class vtkSMProxyIterator;
class vtkSMProxyLocator;
class vtkSMProxyManagerObserver;
class vtkSMProxyManagerProxySet;
class vtkSMProxySelectionModel;
class vtkSMStateLoader;
class vtkStringList;

struct vtkSMSessionProxyManagerInternals;
struct vtkClientServerID;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSessionProxyManager : public vtkObject
{
public:
  //@{
  /**
   * vtkSMSessionProxyManager requires a session and cannot be created without
   * one.
   */
  static vtkSMSessionProxyManager* New(vtkClientSession* session);
  vtkTypeMacro(vtkSMSessionProxyManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Get the vtkClientSession that this proxy manager is for. Each
   * vtkClientSession has exactly one vtkSMProxyManager.
   */
  vtkClientSession* GetSession() const;

  /**
   * Given a group and proxy name, create and return a proxy instance.
   * The user has to delete the proxy when done.
   * NOTE: If this method is called from a scripting language, it may
   * not be possible to delete the returned object with Delete.
   * The VTK wrappers handle New and Delete specially and may not allow
   * the deletion of object created through other methods. Use
   * UnRegister instead.
   */
  vtkSMProxy* NewProxy(
    const char* groupName, const char* proxyName, const char* subProxyName = nullptr);

  /**
   * Returns a vtkSMDocumentation object with the documentation
   * for the proxy with given name and group name. Note that the name and group
   * name are not those with the which the proxy is registered, but those
   * with which the proxy is created i.e. the arguments used for NewProxy().
   */
  vtkSMDocumentation* GetProxyDocumentation(const char* groupName, const char* proxyName);

  /**
   * Returns a vtkSMDocumentation object with the documentation
   * for the given property of the proxy with given name and group name.
   * Note that the name and group
   * name are not those with the which the proxy is registered, but those
   * with which the proxy is created i.e. the arguments used for NewProxy().
   * Also, the property name is the name of an exposed property.
   */
  vtkSMDocumentation* GetPropertyDocumentation(
    const char* groupName, const char* proxyName, const char* propertyName);

  /**
   * Used to pass the control of the proxy to the manager. The user code can
   * then release its reference count and not care about what happens
   * to the proxy. Managed proxies are deleted at destruction. NOTE:
   * The name has to be unique (per group). If not, the existing proxy will be
   * replaced (and unregistered). The proxy instances are grouped in collections
   * (not necessarily the same as the group in the XML configuration file).
   * These collections can be used to separate proxies based on their
   * functionality. For example, implicit planes can be grouped together
   * and the acceptable values of a proxy property can be restricted
   * (using a domain) to this collection.
   */
  void RegisterProxy(const char* groupname, const char* name, vtkSMProxy* proxy);

  /**
   * This overload register the proxy using an unique name returned by
   * GetUniqueProxyName() and returns the name used.
   */
  std::string RegisterProxy(const char* groupname, vtkSMProxy* proxy);

  //@{
  /**
   * Given its name (and group) returns a proxy. If not a managed proxy,
   * returns 0.
   */
  vtkSMProxy* GetProxy(const char* groupname, const char* name);
  vtkSMProxy* GetProxy(const char* name);
  //@}

  /**
   * Returns all proxies registered under the given group with the given name.
   * The collection is cleared before the proxies are added to it.
   */
  void GetProxies(const char* groupname, const char* name, vtkCollection* collection);
  void GetProxies(const char* groupname, vtkCollection* collection)
  {
    this->GetProxies(groupname, nullptr, collection);
  }

  /**
   * Returns the prototype proxy for the given type. This method may create
   * a new prototype proxy, if one does not already exist.
   *
   * @note After loading a plugin, all existing prototypes are discarded. This
   * is done because plugins can potentially alter definitions for existing
   * proxies.
   */
  vtkSMProxy* GetPrototypeProxy(const char* groupname, const char* name);

  /**
   * Returns the number of proxies in a group.
   */
  unsigned int GetNumberOfProxies(const char* groupname) const;

  /**
   * Given a group and an index, returns the name of a proxy.
   * NOTE: This operation is slow.
   */
  const char* GetProxyName(const char* groupname, unsigned int idx);

  /**
   * Given a group and a proxy, return it's name. The proxy may be registered
   * as two different names under the same group, this only returns the  first hit
   * name, use GetProxyNames() to obtain the list of names for the proxy.
   * NOTE: This operation is slow.
   */
  const char* GetProxyName(const char* groupname, vtkSMProxy* proxy);

  /**
   * Given a group and a proxy, return all its names. This clears the
   * \c names list before populating it with the names for the proxy under
   * the group.
   * NOTE: This operation is slow.
   */
  void GetProxyNames(const char* groupname, vtkSMProxy* proxy, vtkStringList* names);

  /**
   * Given a group, returns a name not already used for proxies registered in
   * the given group. The prefix is used to come up with a new name.
   * if alwaysAppend is true, then a suffix will always be appended, if not,
   * the prefix may be used directly if possible.
   */
  std::string GetUniqueProxyName(
    const char* groupname, const char* prefix, bool alwaysAppend = true);

  /**
   * If the proxy is in the given group, return its name, otherwise
   * return nullptr. NOTE: Any following call to proxy manager might make
   * the returned pointer invalid.
   */
  const char* IsProxyInGroup(vtkSMProxy* proxy, const char* groupname);

  //@{
  /**
   * Given its name, unregisters a proxy and removes it from the list
   * of managed proxies.
   */
  void UnRegisterProxy(const char* groupname, const char* name, vtkSMProxy*);
  //@}

  /**
   * Given a proxy, unregisters it. This method unregisters the proxy
   * from all the groups in which it has been registered.
   */
  void UnRegisterProxy(vtkSMProxy* proxy);

  /**
   * Unregisters all managed proxies.
   */
  void UnRegisterProxies();

  /**
   * Get the number of registered links with the server manager.
   */
  int GetNumberOfLinks();

  /**
   * Get the name of a link.
   */
  const char* GetLinkName(int index);

  /**
   * Register proxy/property links with the server manager. The linknames
   * must be unique, if a link with the given name already exists, it will be replaced.
   */
  void RegisterLink(const char* linkname, vtkSMLink* link);

  /**
   * Unregister a proxy or property link previously registered with the given name.
   */
  void UnRegisterLink(const char* linkname);

  /**
   * Get the link registered with the given name. If no such link exists,
   * returns nullptr.
   */
  vtkSMLink* GetRegisteredLink(const char* linkname);

  /**
   * Get the name of the given registered link. If no such link exists,
   * returns nullptr.
   */
  const char* GetRegisteredLinkName(vtkSMLink* link);

  /**
   * Unregister all registered proxy/property links.
   */
  void UnRegisterAllLinks();

  /**
   * Register a custom proxy definition with the proxy manager.
   * This can be a compound proxy definition (look at
   * vtkSMCompoundSourceProxy.h) or a regular proxy definition.
   * For all practical purposes, there's no difference between a proxy
   * definition added using this method or by parsing a server manager
   * configuration file.
   */
  void RegisterCustomProxyDefinition(const char* group, const char* name, vtkPVXMLElement* top);

  /**
   * Given its name, unregisters a custom proxy definition.
   * Note that this can only be used to remove definitions added using
   * RegisterCustomProxyDefinition(), cannot be used to remove definitions
   * loaded using vtkSMXMLParser.
   */
  void UnRegisterCustomProxyDefinition(const char* group, const char* name);

  /**
   * Unregisters all registered custom proxy definitions.
   * Note that this can only be used to remove definitions added using
   * RegisterCustomProxyDefinition(), cannot be used to remove definitions
   * loaded using vtkSMXMLParser.
   */
  void UnRegisterCustomProxyDefinitions();

  //@{
  /**
   * Load custom proxy definitions and register them.
   */
  void LoadCustomProxyDefinitions(const char* filename);
  void LoadCustomProxyDefinitions(vtkPVXMLElement* root);
  //@}

  /**
   * Save registered custom proxy definitions.
   */
  void SaveCustomProxyDefinitions(vtkPVXMLElement* root);

  //@{
  /**
   * Loads the state of the server manager from XML.
   * If loader is not specified, a vtkSMStateLoader instance is used.
   * When loading XML state, `vtkSMSessionProxyManager::GetInLoadXMLState` will
   * return true.
   */
  void LoadXMLState(const char* filename, vtkSMStateLoader* loader = nullptr);
  void LoadXMLState(
    vtkPVXMLElement* rootElement, vtkSMStateLoader* loader = nullptr, bool keepOriginalIds = false);
  //@}

  /**
   * Indicates if an XML state is currently being loaded. This may be used by
   * the application to limit updates to the GUI while state is being loaded.
   */
  vtkGetMacro(InLoadXMLState, bool);

  /**
   * Save the state of the server manager in XML format in a file.
   * This saves the state of all proxies and properties.
   * Return true if the operation succeeded otherwise return false.
   */
  bool SaveXMLState(const char* filename);

  /**
   * Saves the state of the server manager as XML, and returns the
   * vtkPVXMLElement for the root of the state.
   * Note this method allocates a new vtkPVXMLElement object,
   * it's the caller's responsibility to free it by calling Delete().
   */
  vtkPVXMLElement* SaveXMLState();

  //@{
  /**
   * Returns the XML state for the proxy manager. If a non-empty set of proxies
   * is passed, then state is limited to those chosen proxies.
   * If forceRestriction is true, then state will be limited even when the
   * restrictionSet is non-empty.
   *
   * Unlike `SaveXMLState`, this does not fire the `vtkCommand::SaveStateEvent`.
   * This API is primarily intended for use-cases where complete application XML
   * state is not being saved.
   */
  vtkSmartPointer<vtkPVXMLElement> GetXMLState() { return this->GetXMLState({}); }
  vtkSmartPointer<vtkPVXMLElement> GetXMLState(
    const std::set<vtkSMProxy*>& restrictionSet, bool forceRestriction = false);
  //@}

  /**
   * Save/Load registered link states.
   */
  void SaveRegisteredLinks(vtkPVXMLElement* root);

  /**
   * Given a group name, create prototypes and store them
   * in a instance group called groupName_prototypes.
   * Prototypes have their ConnectionID set to the SelfConnection.
   */
  void InstantiateGroupPrototypes(const char* groupName);

  /**
   * Creates protytpes for all known proxy types.
   */
  void InstantiatePrototypes();

  /**
   * Converse on `InstantiatePrototypes`, clear all prototypes.
   */
  void ClearPrototypes();

  /**
   * Return true if the XML Definition was found by vtkProxyDefinitionManager
   */
  bool HasDefinition(const char* groupName, const char* proxyName);

  /**
   * Get if there are any registered proxies that have their properties in
   * a modified state.
   */
  int AreProxiesModified();

  //@{
  /**
   * The server manager configuration XML may define \c \<Hints/\> element for
   * a proxy/property. Hints are metadata associated with the
   * proxy/property. The Server Manager does not (and should not) interpret
   * the hints. Hints provide a mechanism to add GUI-pertinent information
   * to the server manager XML.  Returns the XML element for the hints
   * associated with this proxy/property, if any, otherwise returns nullptr.
   */
  vtkPVXMLElement* GetProxyHints(const char* xmlgroup, const char* xmlname);
  vtkPVXMLElement* GetPropertyHints(
    const char* groupName, const char* proxyName, const char* propertyName);
  //@}

  /**
   * Loads server-manager configuration xml.
   */
  bool LoadConfigurationXML(const char* xmlcontents);

  ///@{
  /**
   * Get the proxy definition manager.
   * Proxy definition manager maintains all the information about proxy
   * definitions.
   */
  vtkProxyDefinitionManager* GetProxyDefinitionManager() const;
  ///@}

  /**
   * Get a registered selection model. Will return nullptr if no such model is
   * registered.
   * This will forward the call to the ProxyManager singleton
   */
  vtkSMProxySelectionModel* GetSelectionModel(const char* name);

  /**
   * Return the number of Selections models registered
   */
  vtkIdType GetNumberOfSelectionModel();

  /**
   * Return the selection model present at the index idx.
   */
  vtkSMProxySelectionModel* GetSelectionModelAt(int idx);

  //@{
  /**
   * Register/UnRegister a selection model. A selection model can be typically
   * used by applications to keep track of active sources, filters, views etc.
   * This will forward the call to the ProxyManager singleton
   */
  void RegisterSelectionModel(const char* name, vtkSMProxySelectionModel*);
  void UnRegisterSelectionModel(const char* name);
  //@}

  /**
   * Find proxy of the group type (xmlgroup, xmltype) registered under a
   * particular group (reggroup). Returns the first proxy found, if any.
   */
  vtkSMProxy* FindProxy(const char* reggroup, const char* xmlgroup, const char* xmltype);

  /**
   * Find a proxy given its global id.
   */
  vtkSMProxy* FindProxy(vtkTypeUInt32 gid) const;

  ///@{
  /**
   * Various structs with meta that are passed as callData when events are
   * fired.
   */
  struct RegisteredProxyInformation
  {
    vtkSMProxy* Proxy;
    const char* GroupName;
    const char* ProxyName;
    // Set when the register/unregister event if fired for registration of
    // a compound proxy definition.
    unsigned int Type;

    enum
    {
      PROXY = 0x1,
      COMPOUND_PROXY_DEFINITION = 0x2,
      LINK = 0x3,
    };
  };

  struct ModifiedPropertyInformation
  {
    vtkSMProxy* Proxy;
    const char* PropertyName;
  };

  struct LoadStateInformation
  {
    vtkPVXMLElement* RootElement;
    vtkSMProxyLocator* ProxyLocator;
  };

  struct StateChangedInformation
  {
    vtkSMProxy* Proxy;
    vtkPVXMLElement* StateChangeElement;
  };
  ///@}
protected:
  vtkSMSessionProxyManager();
  ~vtkSMSessionProxyManager() override;

  friend class vtkSMProxy;
  friend class vtkPVProxyDefinitionIterator;
  friend class vtkSMProxyIterator;
  friend class vtkSMProxyManagerObserver;

  /**
   * Given an XML element and group name create a proxy
   * and all of it's properties.
   */
  vtkSMProxy* NewProxy(vtkPVXMLElement* element, const char* groupname, const char* proxyname,
    const char* subProxyName = nullptr);

  /**
   * Given the proxy name and group name, returns the XML element for
   * the proxy.
   */
  vtkSmartPointer<vtkPVXMLElement> GetProxyElement(
    const char* groupName, const char* proxyName, const char* subProxyName = nullptr);

  /**
   * Handles events.
   */
  virtual void ExecuteEvent(vtkObject* obj, unsigned long event, void* data);

  /**
   * Removes a prototype. Used internally to cleanup obsolete prototypes.
   */
  void RemovePrototype(const char* groupname, const char* proxyname);

private:
  vtkSMSessionProxyManager(const vtkSMSessionProxyManager&) = delete;
  void operator=(const vtkSMSessionProxyManager&) = delete;
  static vtkSMSessionProxyManager* New();

  void MarkPipelineUpdated(vtkSMProxy* proxy) const;

  friend class vtkSMSessionProxyManagerInternals;
  std::unique_ptr<vtkSMSessionProxyManagerInternals> Internals;
  bool InLoadXMLState;
};

#endif

// VTK-HeaderTest-Exclude: vtkSMSessionProxyManager.h
