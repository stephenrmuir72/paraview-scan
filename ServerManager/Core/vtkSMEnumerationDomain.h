/*=========================================================================

  Program:   ParaView
  Module:    vtkSMEnumerationDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMEnumerationDomain
 * @brief   list of integers with associated strings
 *
 * vtkSMEnumerationDomain represents an enumeration of integer values
 * with associated descriptive strings.
 *
 * A typical enumeration domain is described as follows in the servermanager
 * configuration xmls.
 *
 * @code{xml}
 *
 *  <IntVectorProperty ...>
 *    <EnumerationDomain name="enum">
 *      <Entry text="PNG" value="0"/>
 *      <Entry text="JPEG" value="1"/>
 *      ...
 *    </EnumerationDomain>
 *  </IntVectorProperty>
 *
 * @endcode
 *
 * Where, `value` is the integral value to use to set the element on the
 * property and `text` is the descriptive text used in UI and Python script.
 *
 * Starting with ParaView 5.5, the `info` attribute is supported on an `Entry`
 * The value is an additional qualifier for the entry that used in UI to explain
 * the item e.g.
 *
 * @code{xml}
 *
 *  <IntVectorProperty name="Quality">
 *    <EnumerationDomain name="enum">
 *      <Entry text="one" value="0" info="no compression" />
 *      ...
 *      <Entry text="ten" value="10" info="max compression" />
 *      ...
 *    </EnumerationDomain>
 *  </IntVectorProperty>*
 * @endcode
 *
 * If `info` is specified and non-empty, then the UI will show that text in the
 * combo-box rendered in addition to the `text`. `info` has no effect on  the
 * Python API i.e.
 *
 * @code{python}
 *
 *  # either of the following is acceptable, note how `info` has no effect.
 *  writer.Quality = "ten"
 *  # or
 *  writer.Quality = 10
 *
 * @endcode
 */

#ifndef vtkSMEnumerationDomain_h
#define vtkSMEnumerationDomain_h

#include "vtkSMDomain.h"

#include <utility> // for std::pair
#include <vector>  //  for std::vector

class VTKSERVERMANAGERCORE_EXPORT vtkSMEnumerationDomain : public vtkSMDomain
{
public:
  static vtkSMEnumerationDomain* New();
  vtkTypeMacro(vtkSMEnumerationDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the value of the property is in the domain.
   * The property has to be a vtkSMIntVectorProperty. If all
   * vector values are in the domain, it returns 1. It returns
   * 0 otherwise.
   */
  int IsInDomain(vtkSMProperty* property) const override;

  /**
   * Returns true if the int is in the domain. If value is
   * in domain, it's index is return in idx.
   */
  int IsInDomain(int val, unsigned int& idx) const;

  /**
   * Returns the number of entries in the enumeration.
   */
  unsigned int GetNumberOfEntries() const;

  /**
   * Returns the integer value of an enumeration entry.
   */
  int GetEntryValue(unsigned int idx) const;

  /**
   * Returns the descriptive string of an enumeration entry.
   */
  const char* GetEntryText(unsigned int idx) const;

  /**
   * Returns the text for an enumeration value.
   */
  const char* GetEntryTextForValue(int value) const;

  /**
   * Return 1 is the text is present in the enumeration, otherwise 0.
   */
  int HasEntryText(const char* text) const;

  /**
   * Get the value for an enumeration text. The return value is valid only is
   * HasEntryText() returns 1.
   */
  int GetEntryValueForText(const char* text) const;

  /**
   * Given an entry text, return the integer value.
   * Valid is set to 1 if text is defined, otherwise 0.
   * If valid=0, return value is undefined.
   */
  int GetEntryValue(const char* text, int& valid) const;

  /**
   * Returns the `info` text for an enumeration entry.
   *
   * @param[in] idx - index for the entry (not to confused with `value`).
   * @returns info-text, if non-empty else `nullptr`.
   */
  const char* GetInfoText(unsigned int idx) const;

  /**
   * Add a new enumeration entry. text cannot be null.
   */
  void AddEntry(const char* text, int value, const char* info = nullptr);

  /**
   * Clear all entries.
   */
  void RemoveAllEntries();

  /**
   * Update self based on the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  void Update() override;

  //@{
  /**
   * Overridden to ensure that the property's default value is valid for the
   * enumeration, if not it will be set to the first enumeration value.
   */
  bool SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) const override;

protected:
  vtkSMEnumerationDomain();
  ~vtkSMEnumerationDomain() override;
  //@}

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  bool ReadXMLAttributes(const pugi::xml_node& node) override;
  vtkJson SaveState(bool verbose) const override;

private:
  vtkSMEnumerationDomain(const vtkSMEnumerationDomain&) = delete;
  void operator=(const vtkSMEnumerationDomain&) = delete;

  struct vtkEDInternals;
  std::unique_ptr<vtkEDInternals> EInternals;
};

#endif
