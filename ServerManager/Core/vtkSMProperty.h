/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProperty
 * @brief   superclass for all SM properties
 *
 * Each instance of vtkSMProperty or a sub-class represents a method
 * and associated arguments (if any) of a VTK object stored on one
 * or more client manager or server nodes. It may have a state and can push
 * this state to the vtk object it refers to. vtkSMProperty only supports
 * methods with no arguments. Sub-classes support methods with different
 * arguments types and numbers.
 *
 * vtkSMProperty is typically meant for pushing its values to a VTK object.
 * However, a property may be marked as an InformationOnly property
 * in which case its values are obtained from the server with the
 * UpdateInformation() call.
 *
 * Each non-information property can have one or more domains. A domain represents a
 * set of acceptable values the property can have. Domains provide applications
 * mechanisms to extract semantic information from a property.
 *
 * A property has two kinds of values: regular (or checked) values and unchecked
 * values. Regular values are the ones that are pushed to the VTK object when
 * the property is updated. These are the ones that get saved in state, etc.
 * Unchecked values are provided so that domains can update their conditions
 * without having the change the property's value e.g. if the domain range for the
 * IsoContour property changes based on the value of the ArrayName property
 * which selects the array to contour with, the can set the unchecked value on
 * the ArrayName property to each of the available arrays to determine what the
 * domain would be without having to modify the property and update its VTK
 * object. If a property has no unchecked-values explicitly set, then the "Get"
 * methods that access the unchecked-values should simply return the checked
 * values. If the checked values are changed, the unchecked values are reset to
 * match the checked values.
 *
 * A property fires the following events:
 *
 * \li \b vtkCommand::ModifiedEvent : fired when property's value(s) is(are)
 *        modified. This must be fired only when values are really changed, not
 *        just the "set" methods are called. This event must be fired no matter
 *        how the property's values are changed.
 *
 * \li \b vtkCommand::UncheckedPropertyModifiedEvent : fired when the property's
 *        unchecked-value(s) is(are) changed. Note that when a property's
 *        checked values change, it's unchecked values are reset to match the
 *        checked values, so technically,
 *        vtkCommand::UncheckedPropertyModifiedEvent must be fired every time
 *        vtkCommand::ModifiedEvent is fired.
 *
 * \li \b vtkCommand::DomainModifiedEvent : fired when any of this properties
 *        domain's fire the same event.
 *
 * Properties are typically constructed from ServerManager XML configuration
 * files. Attributes available on a Property XML are as follows:
 *
 * \li \b name: \c string: This is the name for the property. This typically
 *        ends up being the name used by the Proxy to refer to this property. It
 *        must be unique for all properties on a Proxy.
 *
 * \li \b label: \c string:This is the user-friendly label. Ideally, the label
 *        should be same as the name, however traditionally that hasn't been the
 *        case.
 *
 * \li \b command: \c string: This is the name of the method to call on the VTK
 *        update for the property.
 *
 * \li \b repeatable or \b repeat_command: \c {0,1}: This used to indicate that
 *        the command can be called repeatedly to update the VTK object. e.g.
 *        for multiple inputs, one must call AddInput(..) repeatedly. It also
 *        implies that the number of elements/items in the property can change.
 *
 * \li \b information_only: \c {0,1}: When set, it implies that this property
 *        is used to obtain values from the VTK object, rather than the default
 *        behavior which is to set values on the VTK object.
 *
 * \li \b information_property: \c string: Value is the name of the property on the
 *        proxy to which this property belongs that can is information_only
 *        property corresponding to this. This is useful when the variable that
 *        this property sets can be changed by other means besides this property
 *        e.g. through interaction. Applications can use this information to
 *        update the value of this property to reflect the VTK-side state.
 *
 * \li \b immediate_update: \c {0,1}: When set, the Proxy will attempt to push
 *        the value for this property to the VTK object as soon as the property
 *        is changed. It is no longer common and should be avoided. It may be
 *        deprecated in near future.
 *
 * \li \b state_ignored: \c {0,1}: When set, changes to this property are not
 *        captured in undo-redo stacks. Unlike is_internal, the value for this
 *        property is saved in state files.
 *
 * \li \b ignore_synchronization: \c {0,1}: When set, changes to this property
 *        are not synchronized among client-processes in collaborative mode.
 *
 * \li \b is_internal: \c {0,1}: When set, the property is treated as internal
 *        which implies that it will not be shown in the UI; its value will not
 *        be pushed when the proxy is created, nor saved in state files or
 *        undo-redo stacks.
 *
 * \li \b animateable: \c {0,1}: When set, the property is considered as
 *        animatable which the UI can use to build the animation interface.
 *
 * \li \b panel_visibility: \c {default,advanced,never}: Indicates to the UI
 *        that the widget corresponding to this property should be shown in the
 *        default or advanced mode, or never at all.
 *
 * \li \b panel_widget: \c string: provides a hint to the UI to determine which
 *        widget to create to edit this property.
 *
 * \li \b no_custom_default: \c {0,1}: When set, vtkSMSettings will neither
 *        change the value of a property upon creation nor save the property
 *        value as a default.
 *
 * \li \b disable_sub_trace: \c {0,1}: When set on a ProxyProperty, the python
 *        trace generated by smtrace.py will not contain the properties of the
 *        proxy pointed by the proxy property.
 *
 */

#ifndef vtkSMProperty_h
#define vtkSMProperty_h

#include "vtkJsonFwd.h" // for vtkJson
#include "vtkObject.h"
#include "vtkSMDomainRange.h"           // for vtkSMDomainRange
#include "vtkServerManagerCoreModule.h" //needed for exports

#include <vtk_pugixml.h> // for pugi

#include <memory> // for std::unique_ptr

class vtkClientSession;
class vtkSMDocumentation;
class vtkSMDomain;
class vtkSMDomainIterator;
class vtkSMPropertyLink;
class vtkSMProxy;
class vtkSMProxyManager;
class vtkSMStateContext;

class VTKSERVERMANAGERCORE_EXPORT vtkSMProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkSMProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum PropertyTypes
  {
    INVALID,
    INT,
    DOUBLE,
    ID_TYPE,
    STRING,
    PROXY,
    INPUT
  };

  /**
   * Returns the name assigned to the property. Used to get the property
   * from a proxy. Note that the name used to obtain a property
   * that is on a subproxy may be different from the XMLName of the property,
   * see the note on ExposedProperties for vtkSMProxy.
   */
  const std::string& GetName() const;

  /**
   * The label for the property. This can either be the one assigned by the XML
   * definition or auto-generated from its named.
   */
  std::string GetLabel() const;

  /**
   * Get if the property is animateable. Non-animateable
   * properties are shown in the GUI only in advanced mode.
   */
  bool GetIsAnimateable() const;

  /**
   * Is InformationOnly is set to true, this property is used to
   * get information from server instead of setting values.
   */
  bool GetIsInformationOnly() const;

  /**
   * Get the associated information property. This allows applications
   * to have access to both the in and out properties. The information
   * property has to be specified in the xml configuration file.
   */
  vtkSMProperty* GetInformationProperty() const;

  /**
   * Get if the property is internal to server manager.
   * Internal properties are not saved in state and should not be
   * displayed in the user interface.
   */
  bool GetIsInternal() const;

  /**
   * Gets whether the property should ignore custom default settings.
   */
  bool GetNoCustomDefault() const;

  /**
   * Gets the panel visibility for the property. The value can be
   * one of:
   * * "default": Show the property by default.
   * * "advanced": Only show the property in the advanced view.
   * * "never": Never show the property on the panel.
   *
   * By default, standard properties have "default" visibility,
   * while information_only properties have "never" visibility.
   */
  const std::string& GetPanelVisibility() const;

  /**
   * Returns which representation type the property will be shown by
   * default for.
   */
  const std::string& GetPanelVisibilityDefaultForRepresentation() const;

  /**
   * Returns name of the panel widget for the property.
   */
  const std::string& GetPanelWidget() const;

  /**
   * Sets the tracing of sub property of this property
   */
  bool GetDisableSubTrace() const;

  /**
   * Returns the proxy to which this property belongs.
   */
  vtkSMProxy* GetParentProxy() const;

  /**
   * Returns true if the property is resizable. A resizable property typically
   * accepts multiple values which can grow or shrink as necessary.
   */
  bool GetIsResizable() const;

  /**
   * Returns the number of elements in the property.
   */
  virtual size_t GetNumberOfElements() const;

  /**
   * Returns the number of unchecked elements in the property.
   */
  virtual size_t GetNumberOfUncheckedElements() const;

  /**
   * For a resizable property (GetIsResizable() == true), this returns the
   * size of the tuple. For example, for a property that takes varying number of
   * tuples of size 3, this will return 3. Default is 1.
   */
  size_t GetNumberOfComponents() const;

  /**
   * Clears unchecked element values, if any.
   */
  virtual void ClearUncheckedElements(){};

  /**
   * Copy values from source property. This copies both checked and unchecked
   * values, firing appropriate events.
   */
  virtual void Copy(const vtkSMProperty* source){};

  /**
   * Returns the session this property belongs to.
   */
  vtkClientSession* GetSession() const;

  /**
   * Returns the proxy manager for the session this property belongs to.
   */
  vtkSMProxyManager* GetProxyManager() const;

  /**
   * returns the PropertyTypes given the classname.
   */
  static int GetPropertyType(const std::string& className);

  /**
   * Returns the type elements this property holds.
   */
  virtual int GetElementType() const = 0;

  ///@{
  /**
   * Reset property value to defaults.
   *
   * `ResetToXMLDefaults` will reset the value those specified in XML if any.
   * If no defaults were specified, then the value is left unchanged. For
   * properties with information property specified, the default implementation
   * copies the value from the information property.
   *
   * `ResetToDomainDefaults` will iterate over all domains in order of definition
   * and call `vtkSMDomain::SetDefaultValues` until the first domain that
   * returns true.
   *
   * `ResetToDefaults` will call ResetToDomainDefaults first and if none of the
   * domains supports resetting the property value, then `ResetToXMLDefaults` is
   * called.
   */
  virtual bool ResetToXMLDefaults(bool useUnchecked = false);
  virtual bool ResetToDomainDefaults(bool useUnchecked = false);
  bool ResetToDefaults(bool useUnchecked = false);
  ///@}

  vtkSMDomainRange GetDomains();

protected:
  vtkSMProperty();
  ~vtkSMProperty() override;

  /**
   * Same as `Modified`, except to be called when unchecked values of the
   * property are changed.
   */
  virtual void UncheckedPropertyModified();

  ///@{
  friend class vtkSMProxy;
  void SetParentProxy(vtkSMProxy* proxy);
  void SetName(const std::string& name);
  void SetLabel(const std::string& label);
  void AddDomain(vtkSMDomain* domain);
  const std::string& GetInformationPropertyName() const;
  void SetInformationProperty(vtkSMProperty* property);
  virtual bool ReadXMLAttributes(const pugi::xml_node& node);
  virtual vtkJson SaveState(bool verbose) const;
  virtual bool LoadState(const vtkJson& json, vtkSMStateContext* context);
  ///@}

  ///@{
  friend class vtkSMDomain;
  void AddDependentDomain(vtkSMDomain* domain);
  ///@}

  /**
   * Updates all domains that depend on this property.
   */
  void UpdateDependentDomains() const;

private:
  vtkSMProperty(const vtkSMProperty&) = delete;
  void operator=(const vtkSMProperty&) = delete;

  void HandleDomainModifiedEvent();

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
