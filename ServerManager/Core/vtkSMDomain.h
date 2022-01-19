/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMDomain
 * @brief   represents the possible values a property can have
 *
 * vtkSMDomain is an abstract class that describes the "domain" of a
 * a widget. A domain is a collection of possible values a property
 * can have.
 *
 * Each domain can depend on one or more properties to compute it's
 * values. This are called "required" properties and can be set in
 * the XML configuration file.
 *
 * Every time a domain changes it must fire a vtkCommand::DomainModifiedEvent.
 * Applications may decide to update the UI every-time the domain changes. As a
 * result, domains ideally should only fire that event when their values change
 * for real not just potentially changed.
 * @sa vtkSMDomain::DeferDomainModifiedEvents.
 *
 */

#ifndef vtkSMDomain_h
#define vtkSMDomain_h

#include "vtkObject.h"
#include "vtkServerManagerCoreModule.h" //needed for exports

#include <memory>        // for std::unique_ptr
#include <vtkJsonFwd.h>  // for vtkJson
#include <vtk_pugixml.h> // for pugi

class vtkPVDataInformation;
class vtkSMProperty;

class VTKSERVERMANAGERCORE_EXPORT vtkSMDomain : public vtkObject
{
public:
  vtkTypeMacro(vtkSMDomain, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the name for the domain.
   */
  const std::string& GetName() const;

  /**
   * Returns true of the domain should be treated as optional by the
   * application.
   */
  bool GetIsOptional() const;

  /**
   * Returns the vtkSMProperty that this domain is attached to. A domain can be
   * attached to one and only one property.
   */
  vtkSMProperty* GetProperty() const;

  /**
   * Return values for `IsInDomain` calls.
   */
  enum IsInDomainReturnCodes
  {
    NOT_APPLICABLE = -1,
    NOT_IN_DOMAIN = 0,
    IN_DOMAIN = 1,
  };

  /**
   * Is the (unchecked) value of the property in the domain? Overwritten by
   * sub-classes.
   *
   * Returned values as defined in `IsInDomainReturnCodes`. `NOT_APPLICABLE` is
   * returned if the domain is not applicable for the property values.
   * `NOT_IN_DOMAIN` implies that the value is not in domain while `IN_DOMAIN`
   * implies that the value is acceptable.
   *
   */
  virtual int IsInDomain(vtkSMProperty* vtkNotUsed(property)) const { return IN_DOMAIN; }

  /**
   * Update the domain based on the "unchecked" values of all required
   * properties.
   */
  virtual void Update() {}

  /**
   * A vtkSMProperty is often defined with a default value in the
   * XML itself. However, many times, the default value must be determined
   * at run time. To facilitate this, domains can override this method
   * to compute and set the default value for the property.
   * Note that unlike the compile-time default values, the
   * application must explicitly call this method to initialize the
   * property.
   * If \c use_unchecked_values is true, the property's unchecked values will be
   * changed by this method.
   * Returns false if the domain updated the property.
   * Default implementation does nothing.
   */
  virtual bool SetDefaultValues(vtkSMProperty*, bool vtkNotUsed(use_unchecked_values)) const
  {
    return false;
  }

protected:
  vtkSMDomain();
  ~vtkSMDomain() override;

  /**
   * Returns a given required property of the given function. A domain can have
   * zero or more required properties. A required property is a property that
   * impacts the domain in some way. All required properties as assigned a
   * "role" or "function" which helps determine how that property affects the
   * particular domain.
   */
  vtkSMProperty* GetRequiredProperty(const std::string& function) const;

  /**
   * Returns the number of required properties.
   */
  unsigned int GetNumberOfRequiredProperties() const;

  ///@{
  friend class vtkSMProxy;
  friend class vtkSMProperty;
  void SetProperty(vtkSMProperty* property);
  virtual bool ReadXMLAttributes(const pugi::xml_node& node);
  virtual vtkJson SaveState(bool verbose) const;
  ///@}

private:
  vtkSMDomain(const vtkSMDomain&) = delete;
  void operator=(const vtkSMDomain&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
