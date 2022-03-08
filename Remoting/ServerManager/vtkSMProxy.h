/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxy
 * @brief   proxy for a VTK object(s) on a server
 *
 * vtkSMProxy manages VTK object(s) that are created on a server
 * using the proxy pattern. The managed object is manipulated through
 * properties.
 * The type of object created and managed by vtkSMProxy is determined
 * by the VTKClassName variable. The object is managed by getting the desired
 * property from the proxy, changing it's value and updating the server
 * with UpdateVTKObjects().
 * A proxy can be composite. Sub-proxies can be added by the proxy
 * manager. This is transparent to the user who sees all properties
 * as if they belong to the root proxy.
 *
 * A proxy keeps an iVar ConnectionID. This is the connection ID for the
 * connection on which this proxy exists. Currently, since a ParaView
 * client is connected to 1 and only 1 server. This ID is
 * insignificant. However, it provides the ground work to enable a client
 * to connect with multiple servers.  ConnectionID must be set immediately
 * after instantiating the proxy (if at all).  Changing the ConnectionID
 * after that can be dangerous.
 *
 * Once a proxy has been defined, it can be listed in another secondary group
 * \code
 * <ProxyGroup name="new_group">
 *  < Proxy group = "group" name ="proxyname" />
 * </ProxyGroup>
 * \endcode
 *
 * When defining a proxy in the XML configuration file,
 * to derive the property interface from another proxy definition,
 * we can use attributes "base_proxygroup" and "base_proxyname" which
 * identify the proxy group and proxy name of another proxy. Base interfaces
 * can be defined recursively, however care must be taken to avoid cycles.
 *
 * There are several special XML features available for subproxies.
 * \li 1) It is possible to share properties among subproxies.
 *    eg.
 *    \code
 *    <Proxy name="Display" class="Alpha">
 *      <SubProxy>
 *        <Proxy name="Mapper" class="vtkPolyDataMapper">
 *          <InputProperty name="Input" ...>
 *            ...
 *          </InputProperty>
 *          <IntVectorProperty name="ScalarVisibility" ...>
 *            ...
 *          </IntVectorProperty>
 *            ...
 *        </Proxy>
 *      </SubProxy>
 *      <SubProxy>
 *        <Proxy name="Mapper2" class="vtkPolyDataMapper">
 *          <InputProperty name="Input" ...>
 *            ...
 *          </InputProperty>
 *          <IntVectorProperty name="ScalarVisibility" ...>
 *            ...
 *          </IntVectorProperty>
 *            ...
 *        </Proxy>
 *        <ShareProperties subproxy="Mapper">
 *          <Exception name="Input" />
 *        </ShareProperties>
 *      </SubProxy>
 *    </Proxy>
 *    \endcode
 *    Thus, subproxies Mapper and Mapper2 share the properties that are
 *    common to both; except those listed as exceptions using the "Exception"
 *    tag.
 *
 * \li 2) It is possible for a subproxy to use proxy definition defined elsewhere
 *     by identifying the interface with attributes "proxygroup" and "proxyname".
 *     eg.
 *     \code
 *     <SubProxy>
 *       <Proxy name="Mapper" proxygroup="mappers" proxyname="PolyDataMapper" />
 *     </SubProxy>
 *     \endcode
 *
 * \li 3) It is possible to scope the properties exposed by a subproxy and expose
 *     only a fixed set of properties to be accessible from outside. Also,
 *     while exposing the property, it can be exposed with a different name.
 *     eg.
 *     \code
 *     <Proxy name="Alpha" ....>
 *       ....
 *       <SubProxy>
 *         <Proxy name="Mapper" proxygroup="mappers" proxyname="PolyDataMapper" />
 *         <ExposedProperties>
 *           <Property name="LookupTable" exposed_name="MapperLookupTable" />
 *         </ExposedProperties>
 *       </SubProxy>
 *     </Proxy>
 *     \endcode
 *     Here, for the proxy Alpha, the property with the name LookupTable from its
 *     subproxy "Mapper" can be obtained by calling GetProperty("MapperLookupTable")
 *     on an instance of the proxy Alpha. "exposed_name" attribute is optional, if
 *     not specified, then the "name" is used as the exposed property name.
 *     Properties that are not exposed are treated as
 *     non-saveable and non-animateable (see vtkSMProperty for details).
 *     Exposed property restrictions only work when
 *     using the GetProperty on the container proxy (in this case Alpha) or
 *     using the PropertyIterator obtained from the container proxy. If one
 *     is to some how obtain a pointer to the subproxy and call GetProperty on
 *     it (or get a PropertyIterator for the subproxy), the properties exposed
 *     by the container class are no longer applicable.
 *     If two exposed properties are exposed with the same name, then a Warning is
 *     flagged -- only one of the two exposed properties will get exposed.
 *
 * @sa
 * vtkSMProxyManager vtkSMProperty vtkSMSourceProxy vtkSMPropertyIterator
 */

#ifndef vtkSMProxy_h
#define vtkSMProxy_h

#include "vtkClientSession.h" // for vtkClientSession::DATA_SERVER /RENDER_SERVER
#include "vtkCommand.h"       // for vtkCommand
#include "vtkNJsonFwd.h"      // needed for vtkNJson
#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkWeakPointer.h"                 // for vtkWeakPointer

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkPVInformation;
class vtkPVXMLElement;
class vtkSMDocumentation;
class vtkSMLoadStateContext;
class vtkSMProperty;
class vtkSMPropertyGroup;
class vtkSMPropertyIterator;
class vtkSMProxyLocator;
class vtkSMProxyManager;
class vtkSMProxyObserver;
class vtkSMSessionProxyManager;
struct vtkSMProxyInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMProxy : public vtkObject
{
public:
  static vtkSMProxy* New();
  vtkTypeMacro(vtkSMProxy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Provides access to the proxy manager.
   */
  vtkSMSessionProxyManager* GetSessionProxyManager() const { return this->GetProxyManager(); }
  vtkSMSessionProxyManager* GetProxyManager() const;
  ///@}

  ///@{
  /**
   * Returns the client-session that this proxy belongs to.
   */
  vtkClientSession* GetSession() const;
  ///@}

  ///@{
  /**
   * Get the location where the underlying VTK-objects are created. The
   * value can be constructed by or-ing vtkClientSession::ServiceTypes.
   */
  vtkGetMacro(Location, vtkTypeUInt32);
  ///@}

  /**
   * Returns the global id for this proxy.
   *
   * A global id of 0 indicates a prototype proxy.
   */
  vtkGetMacro(GlobalID, vtkTypeUInt32);

  // Set or override a key/value pair as annotation to that proxy.
  // If the value is nullptr, this method is equivalent to RemoveAnnotation(key)
  void SetAnnotation(const char* key, const char* value);

  /**
   * Retrieve an annotation with a given key.
   * If not found, this will return nullptr.
   */
  const char* GetAnnotation(const char* key);

  /**
   * Remove a given annotation based on its key to the proxy.
   */
  void RemoveAnnotation(const char* key);

  /**
   * Remove all proxy annotations.
   */
  void RemoveAllAnnotations();

  /**
   * Return true if a given annotation exists.
   */
  bool HasAnnotation(const char* key);

  /**
   * Return the number of available annotations.
   */
  int GetNumberOfAnnotations();

  /**
   * Return the nth key of the available annotations.
   */
  const char* GetAnnotationKeyAt(int index);

  /**
   * Return the property with the given name. If no property is found
   * nullptr is returned.
   */
  virtual vtkSMProperty* GetProperty(const char* name)
  {
    return this->GetProperty(name, /*self-only*/ 0);
  }

  /**
   * Return a property of the given name from self or one of
   * the sub-proxies. If selfOnly is set, the sub-proxies are
   * not checked.
   */
  virtual vtkSMProperty* GetProperty(const char* name, int selfOnly);

  /**
   * Given a property pointer, returns the name that was used
   * to add it to the proxy. Returns nullptr if the property is
   * not in the proxy. If the property belongs to a sub-proxy,
   * it returns the exposed name or nullptr if the property is not
   * exposed.
   */
  const char* GetPropertyName(vtkSMProperty* prop);

  /**
   * Update the VTK object on the server by pushing the values of
   * all modified properties (un-modified properties are ignored).
   * If the object has not been created, it will be created first.
   */
  virtual void UpdateVTKObjects();

  /**
   * Recreate the VTK object for this proxy. This is a convenient mechanism
   * to create a new VTK object with the same state as an existing one in its
   * stead.
   */
  virtual void RecreateVTKObjects();

  /**
   * Invoke a command.
   */
  void InvokeCommand(const char* name,
    vtkTypeUInt32 destinationMask = (vtkClientSession::DATA_SERVER |
      vtkClientSession::RENDER_SERVER));

  /**
   * Invoke command obtain command execution status.
   */
  rxcpp::observable<bool> ExecuteCommand(const char* name,
    vtkTypeUInt32 destinationMask = (vtkClientSession::DATA_SERVER |
      vtkClientSession::RENDER_SERVER));

  //@{
  /**
   * Returns the type of object managed by the proxy.
   */
  vtkGetStringMacro(VTKClassName);
  //@}

  //@{
  /**
   * the type of object created by the proxy.
   * This is used only when creating the server objects. Once the server
   * object(s) have been created, changing this has no effect.
   */
  vtkSetStringMacro(VTKClassName);
  //@}

  /**
   * Returns a new (initialized) iterator of the properties.
   */
  virtual vtkSMPropertyIterator* NewPropertyIterator();

  /**
   * Returns the number of consumers. Consumers are proxies
   * that point to this proxy through a property (usually
   * vtkSMProxyProperty)
   */
  unsigned int GetNumberOfConsumers();

  /**
   * Returns the consumer of given index. Consumers are proxies
   * that point to this proxy through a property (usually
   * vtkSMProxyProperty)
   */
  vtkSMProxy* GetConsumerProxy(unsigned int idx);

  /**
   * Returns the corresponding property of the consumer of given
   * index. Consumers are proxies that point to this proxy through
   * a property (usually vtkSMProxyProperty)
   */
  vtkSMProperty* GetConsumerProperty(unsigned int idx);

  /**
   * Returns the number of proxies this proxy depends on (uses or is
   * connected to through the pipeline).
   */
  unsigned int GetNumberOfProducers();

  /**
   * Returns a proxy this proxy depends on, given index.
   */
  vtkSMProxy* GetProducerProxy(unsigned int idx);

  /**
   * Returns the property holding a producer proxy given an index. Note
   * that this is a property of this proxy and it points to the producer
   * proxy.
   */
  vtkSMProperty* GetProducerProperty(unsigned int idx);

  //@{
  /**
   * Assigned by the XML parser. The name assigned in the XML
   * configuration. Can be used to figure out the origin of the
   * proxy.
   */
  vtkGetStringMacro(XMLName);
  //@}

  //@{
  /**
   * Assigned by the XML parser. The group in the XML configuration that
   * this proxy belongs to. Can be used to figure out the origin of the
   * proxy.
   */
  vtkGetStringMacro(XMLGroup);
  //@}

  //@{
  /**
   * Assigned by the XML parser. The label assigned in the XML
   * configuration. This is a more user-friendly name
   * for the proxy, although it's cannot be used to locate the
   * proxy.
   */
  vtkGetStringMacro(XMLLabel);
  //@}

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
   * Use this method to set all properties on this proxy to their default
   * values. This iterates over all properties on this proxy, thus if this proxy
   * had subproxies, this method will iterate over only the exposed properties
   * and call vtkSMProperty::ResetToXMLDefaults().
   */
  virtual void ResetPropertiesToXMLDefaults();

  /**
   * Use this method to set all properties on this proxy to their default
   * domains. This iterates over all properties on this proxy, thus if this proxy
   * had subproxies, this method will iterate over only the exposed properties
   * and call vtkSMProperty::ResetToDomainDefaults().
   */
  virtual void ResetPropertiesToDomainDefaults();

  enum ResetPropertiesMode
  {
    DEFAULT = 0,
    ONLY_XML = 1,
    ONLY_DOMAIN = 2
  };

  /**
   * Use this method to set all properties on this proxy to their default domain
   * or values. This iterates over all properties on this proxy, thus if this
   * proxy had subproxies, this method will iterate over only the exposed
   * properties and call correct reset methods.
   * The parameter allows to choose between resetting ONLY_XML, ONLY_DOMAIN or DEFAULT,
   * ie. reset to domain if available, if not reset to xml.
   * default value is DEFAULT.
   */
  virtual void ResetPropertiesToDefault(ResetPropertiesMode mode = DEFAULT);

  /**
   * Flags used for the proxyPropertyCopyFlag argument to the Copy method.
   */
  enum
  {
    COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE = 0,

    COPY_PROXY_PROPERTY_VALUES_BY_CLONING // < No longer supported!!!
  };

  //@{
  /**
   * Copies values of all the properties and sub-proxies from src.
   * \b NOTE: This does NOT create properties and sub-proxies. Only
   * copies values. Mismatched property and sub-proxy pairs are
   * ignored.
   * Properties of type exceptionClass are not copied. This
   * is usually vtkSMInputProperty.
   * proxyPropertyCopyFlag specifies how the values for vtkSMProxyProperty
   * and its subclasses are copied over: by reference or by
   * cloning (ie. creating new instances of the value proxies and
   * synchronizing their values). This is no longer relevant since we don't
   * support COPY_PROXY_PROPERTY_VALUES_BY_CLONING anymore.
   */
  void Copy(vtkSMProxy* src);
  void Copy(vtkSMProxy* src, const char* exceptionClass);
  virtual void Copy(vtkSMProxy* src, const char* exceptionClass, int proxyPropertyCopyFlag);
  //@}

  //@{
  /**
   * Returns the documentation for this proxy.
   */
  vtkGetObjectMacro(Documentation, vtkSMDocumentation);
  //@}

  //@{
  /**
   * The server manager configuration XML may define \p \<Hints/\> element for a
   * proxy. Hints are metadata associated with the proxy. The Server Manager
   * does not (and should not) interpret the hints. Hints provide a mechanism
   * to add GUI pertinant information to the server manager XML.
   * Returns the XML element for the hints associated with this proxy,
   * if any, otherwise returns nullptr.
   */
  vtkGetObjectMacro(Hints, vtkPVXMLElement);
  //@}

  //@{
  /**
   * Returns if the VTK objects for this proxy have been created.
   */
  vtkGetMacro(ObjectsCreated, int);
  //@}

  /**
   * Given a source proxy, makes this proxy point to the same server-side
   * object (with a new id). This method copies connection id as well as
   * server ids. This method can be called only once on an uninitialized
   * proxy (CreateVTKObjects() also initialized a proxy) This is useful to
   * make two (or more) proxies represent the same VTK object. This method
   * does not copy IDs for any subproxies.
   */
  void InitializeAndCopyFromProxy(vtkSMProxy* source);

  //@{
  /**
   * Gathers information about this proxy.
   * On success, the \c information object is filled up with details about the
   * VTK object.
   */
  bool GatherInformation(vtkPVInformation* information);
  bool GatherInformation(vtkPVInformation* information, vtkTypeUInt32 location);
  //@}

  /**
   * Saves the state of the proxy. This state can be reloaded
   * to create a new proxy that is identical the present state of this proxy.
   * The resulting proxy's XML hieratchy is returned, in addition if the root
   * argument is not nullptr then it's also inserted as a nested element.
   * This call saves all a proxy's properties, including exposed properties
   * and sub-proxies. More control is provided by the following overload.
   */
  virtual vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root);
  /**
   * The iterator is use to filter the property available on the given proxy
   */
  virtual vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root, vtkSMPropertyIterator* iter);

  /**
   * Loads the proxy state from the XML element. Returns 0 on failure.
   * \c locator is used to locate other proxies that may be referred to in the
   * state XML (which happens in case of properties of type vtkSMProxyProperty
   * or subclasses). If locator is nullptr, then such properties are left
   * unchanged.
   */
  virtual int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

  /**
   * A proxy instance can be a sub-proxy for some other proxy. In that case,
   * this method returns true.
   */
  bool GetIsSubProxy();

  /**
   * If this instance is a sub-proxy, this method will return the proxy of which
   * this instance is an immediate sub-proxy.
   */
  vtkSMProxy* GetParentProxy();

  /**
   * Call GetParentProxy() recursively till a proxy that is not a subproxy of
   * any other proxy is found. May return this instance, if this is not a
   * subproxy of any other proxy.
   */
  vtkSMProxy* GetTrueParentProxy();

  /**
   * Returns the property group at \p index for the proxy.
   */
  vtkSMPropertyGroup* GetPropertyGroup(size_t index) const;

  /**
   * Returns the number of property groups that the proxy contains.
   */
  size_t GetNumberOfPropertyGroups() const;

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
  void SetLogName(const char* name);
  vtkGetStringMacro(LogName);
  //@}

  /**
   * A helper that makes up an default name if none is provided.
   */
  const char* GetLogNameOrDefault();

  enum
  {
    AnnotationsChangedEvent = vtkCommand::UserEvent + 900,
  };

protected:
  vtkSMProxy();
  ~vtkSMProxy() override;

  /**
   * Save proxy state.
   *
   * If tstamp is non-zero, then only those properties that have been modified
   * since the tstamp are saved to the state.
   *
   * Setting \p destinationMask to only one of the services, one can save only the
   * properties applicable to this service.
   */
  bool SaveState(vtkNJson& json, vtkMTimeType tstamp = 0,
    vtkTypeUInt32 destinationMask = (vtkClientSession::DATA_SERVER |
      vtkClientSession::RENDER_SERVER));

  /**
   * Load state. `context` is used to locate other proxies, for example, that
   * may be referenced in the state.
   */
  bool LoadState(const vtkNJson& element, vtkSMProxyLocator* locator);

  /**
   * Add a property with the given key (name). The name can then
   * be used to retrieve the property with GetProperty(). If a
   * property with the given name has been added before, it will
   * be replaced. This includes properties in sub-proxies.
   */
  virtual void AddProperty(const char* name, vtkSMProperty* prop);

  //@{
  /**
   * These classes have been declared as friends to minimize the
   * public interface exposed by vtkSMProxy. Each of these classes
   * use a small subset of protected methods. This should be kept
   * as such.
   */
  friend class vtkSMCameraLink;
  friend class vtkSMCompoundProxy;
  friend class vtkSMCompoundSourceProxy;
  friend class vtkSMDeserializerProtobuf;
  friend class vtkSMMultiServerSourceProxy;
  friend class vtkSMNamedPropertyIterator;
  friend class vtkSMOrderedPropertyIterator;
  friend class vtkSMProperty;
  friend class vtkSMPropertyIterator;
  friend class vtkSMProxyObserver;
  friend class vtkSMProxyProperty;
  friend class vtkSMProxyRegisterUndoElement;
  friend class vtkSMProxyUnRegisterUndoElement;
  friend class vtkSMSessionProxyManager;
  friend class vtkSMSourceProxy;
  friend class vtkSMStateLoader;
  friend class vtkSMUndoRedoStateLoader;
  friend class vtkSMViewProxy;
  //@}

  //@{
  /**
   * Assigned by the XML parser. The name assigned in the XML
   * configuration. Can be used to figure out the origin of the
   * proxy.
   */
  vtkSetStringMacro(XMLName);
  //@}

  //@{
  /**
   * Assigned by the XML parser. The group in the XML configuration that
   * this proxy belongs to. Can be used to figure out the origin of the
   * proxy.
   */
  vtkSetStringMacro(XMLGroup);
  //@}

  //@{
  /**
   * Assigned by the XML parser. The label assigned in the XML
   * configuration. This is a more user-friendly name
   * for the proxy, although it's cannot be used to locate the
   * proxy.
   */
  vtkSetStringMacro(XMLLabel);
  //@}

  /**
   * Cleanup code. Remove all observers from all properties assigned to
   * this proxy.  Called before deleting properties.
   * This also removes observers on subproxies.
   */
  void RemoveAllObservers();

  /**
   * Note on property modified flags:
   * The modified flag of each property associated with a proxy is
   * stored in the proxy object instead of in the property itself.
   * Here is a brief explanation of how modified flags are used:
   * \li 1. When a property is modified, the modified flag is set
   * \li 2. In UpdateVTKObjects(), the proxy visits all properties and
   * calls AppendCommandToStream() on each modified property.
   * It also resets the modified flag.

   * The reason why the modified flag is stored in the proxy instead
   * of property is in item 2 above. If multiple proxies were sharing the same
   * property, the first one would reset the modified flag in
   * UpdateVTKObjects() and then others would not call AppendCommandToStream()
   * in their turn. Therefore, each proxy has to keep track of all
   * properties it updated.
   * This is done by adding observers to the properties. When a property
   * is modified, it invokes all observers and the observers set the
   * appropriate flags in the proxies.
   * Changes the modified flag of a property. Used by the observers
   */
  virtual void SetPropertyModifiedFlag(const char* name, int flag);

  /**
   * Add a sub-proxy.
   * If the overrideOK flag is set, then no warning is printed when a new
   * subproxy replaces a preexisting one.
   */
  void AddSubProxy(const char* name, vtkSMProxy* proxy, int overrideOK = 0);

  /**
   * Remove a sub-proxy.
   */
  void RemoveSubProxy(const char* name);

  /**
   * Returns a sub-proxy. Returns 0 if sub-proxy does not exist.
   */
  vtkSMProxy* GetSubProxy(const char* name);

  /**
   * Returns a sub-proxy. Returns 0 if sub-proxy does not exist.
   */
  vtkSMProxy* GetSubProxy(unsigned int index);

  /**
   * Returns the name used to store sub-proxy. Returns 0 if sub-proxy does
   * not exist.
   */
  const char* GetSubProxyName(unsigned int index);

  /**
   * Returns the name used to store sub-proxy. Returns 0 is the sub-proxy
   * does not exist.
   */
  const char* GetSubProxyName(vtkSMProxy*);

  /**
   * Returns the number of sub-proxies.
   */
  unsigned int GetNumberOfSubProxies();

  /**
   * Called by a proxy property, this adds the property,proxy
   * pair to the list of consumers.
   */
  virtual void AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy);

  /**
   * Remove the property,proxy pair from the list of consumers.
   */
  virtual void RemoveConsumer(vtkSMProperty* property, vtkSMProxy* proxy);

  /**
   * Remove all consumers.
   */
  virtual void RemoveAllConsumers();

  /**
   * Called by an proxy/input property to add property, proxy pair
   * to the list of producers.
   */
  void AddProducer(vtkSMProperty* property, vtkSMProxy* proxy);

  /**
   * Remove the property,proxy pair from the list of producers.
   */
  void RemoveProducer(vtkSMProperty* property, vtkSMProxy* proxy);

  /**
   * If a proxy is deprecated, prints a warning.
   */
  bool WarnIfDeprecated();

  //@{
  /**
   * Creates a new property and initializes it by calling ReadXMLAttributes()
   * with the right XML element.
   */
  vtkSMProperty* NewProperty(const char* name);
  vtkSMProperty* NewProperty(const char* name, vtkPVXMLElement* propElement);
  //@}

  /**
   * Links properties such that when inputProperty's checked or unchecked values
   * are changed, the outputProperty's corresponding values are also changed.
   */
  void LinkProperty(vtkSMProperty* inputProperty, vtkSMProperty* outputProperty);

  /**
   * Parses the XML to create a new property group. This can handle
   * \c \<PropertyGroup/\> elements defined in both regular Proxy section or when
   * exposing properties from sub-proxies.
   */
  vtkSMPropertyGroup* NewPropertyGroup(vtkPVXMLElement* propElement);

  /**
   * Adds a property groups.
   */
  void AppendPropertyGroup(vtkSMPropertyGroup* group);

  //@{
  /**
   * Read attributes from an XML element.
   */
  virtual int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element);
  void SetupExposedProperties(const char* subproxy_name, vtkPVXMLElement* element);
  void SetupSharedProperties(vtkSMProxy* subproxy, vtkPVXMLElement* element);
  //@}

  /**
   * Expose a subproxy property from the base proxy. The property with the name
   * "property_name" on the subproxy with the name "subproxy_name" is exposed
   * with the name "exposed_name".
   * If the overrideOK flag is set, then no warning is printed when a new
   * exposed property replaces a preexisting one.
   */
  void ExposeSubProxyProperty(const char* subproxy_name, const char* property_name,
    const char* exposed_name, int overrideOK = 0);

  /**
   * Handle events fired by subproxies.
   */
  virtual void ExecuteSubProxyEvent(vtkSMProxy* o, unsigned long event, void* data);

  virtual int CreateSubProxiesAndProperties(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element);

  /**
   * Internal method used by `SetLogName`
   */
  virtual void SetLogNameInternal(
    const char* name, bool propagate_to_subproxies, bool propagate_to_proxylistdomains);

  /**
   * Trigger an update for the domains of Proxies consuming data of this Proxy
   */
  void UpdateConsumerDomains();

  /**
   * Called by vtkSMSessionProxyManager when piggy-backed information is
   * received. This method should update locally maintained vtkPVInformation
   * subclasses based on the received information(s).
   */
  virtual void ProcessPiggybackedInformation(const vtkNJson& json);

  char* VTKClassName;
  char* XMLGroup;
  char* XMLName;
  char* XMLLabel;
  int ObjectsCreated;
  int DoNotModifyProperty;

  vtkTypeUInt32 GlobalID;
  vtkTypeUInt32 Location;

  /**
   * Flag used to help speed up UpdateVTKObjects and ArePropertiesModified
   * calls.
   */
  bool PropertiesModified;

  /**
   * Indicates if any properties are modified.
   */
  bool ArePropertiesModified();

  void SetHints(vtkPVXMLElement* hints);
  void SetDeprecated(vtkPVXMLElement* deprecated);

  void SetXMLElement(vtkPVXMLElement* element);
  vtkPVXMLElement* XMLElement;

  vtkSMDocumentation* Documentation;
  vtkPVXMLElement* Hints;
  vtkPVXMLElement* Deprecated;

  vtkWeakPointer<vtkSMProxy> ParentProxy;
  vtkWeakPointer<vtkSMSessionProxyManager> ProxyManager;

  vtkSMProxyInternals* Internals;
  vtkSMProxyObserver* SubProxyObserver;
  vtkSMProxy(const vtkSMProxy&) = delete;
  void operator=(const vtkSMProxy&) = delete;

  /**
   * Send a request for the corresponding remote object to be deleted
   */
  void DeleteRemoteObject();

private:
  vtkSMProperty* SetupExposedProperty(vtkPVXMLElement* propertyElement, const char* subproxy_name);

  friend class vtkSMProxyInfo;

  char* LogName;
  std::string DefaultLogName;
};

#endif
