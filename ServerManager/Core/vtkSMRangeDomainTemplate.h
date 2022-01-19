/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRangeDomainTemplate.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMRangeDomainTemplate
 * @brief   superclass for type-specific range domains
 * i.e. domains that constrain a value within a min and max.
 *
 * vtkSMRangeDomainTemplate represents an interval in real space (using
 * precision based on the data-type) specified using a min and a max value.
 * Valid XML attributes are:
 * @verbatim
 * * min
 * * max
 * * resolution
 * @endverbatim
 * Both min and max attributes can have one or more space space
 * separated value arguments.
 * Resolution expects only one value.
 * Optionally, a Required Property may be specified (which typically is a
 * information property) which can be used to obtain the range for the values as
 * follows:
 * @verbatim
 * <DoubleRangeDomain ...>
 *    <RequiredProperties>
 *      <Property name="<InfoPropName>" function="RangeInfo" />
 *    </RequiredProperties>
 * </DoubleRangeDomain>
 * @endverbatim
 *
 * vtkSMRangeDomainTemplate provides a mechanism to control how the default
 * value for any property can be determined using the domain either the min, max
 * or mid of the range. One can do that using the "default_mode" attribute in
 * XML with valid values as "min", "max", "mid", or a comma separated sequence
 * of the three e.g "min,max,min". If none is specified, "mid" is assumed.
 * The comma-separated sequence can be used to set a different mode for each
 * component of the property i.e. "min,max,min" means set element 0 as min,
 * element 1 as max and element 2 and min. If the number of elements on the
 * property is less than the number of default specified, the last value is
 * assumed to be repeated. Thus, "min,max,min" is same as the regular expression
 * "min,max,min(,min)*".
 */

#ifndef vtkSMRangeDomainTemplate_h
#define vtkSMRangeDomainTemplate_h

#include "vtkSMDomain.h"
#include "vtkServerManagerCoreModule.h" //needed for exports
#include <vector>                       // needed for std::vector

template <class T>
class VTKSERVERMANAGERCORE_EXPORT vtkSMRangeDomainTemplate : public vtkSMDomain
{
public:
  vtkTypeMacro(vtkSMRangeDomainTemplate, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the double (val) is in the domain. If value is
   * in domain, it's index is return in idx.
   * A value is in the domain if it is between (min, max)
   */
  bool IsInDomain(size_t idx, T val) const;

  /**
   * Return a min. value if it exists. If the min. exists
   * exists is set to 1. Otherwise, it is set to 0.
   * An unspecified min. is equivalent to -inf
   */
  T GetMinimum(size_t idx, bool* exists = nullptr) const;

  /**
   * Return a max. value if it exists. If the max. exists
   * exists is set to 1. Otherwise, it is set to 0.
   * An unspecified max. is equivalent to +inf
   */
  T GetMaximum(size_t idx, bool* exists = nullptr) const;

  /**
   * Returns the resolution. If not specified -1 is returned.
   */
  int GetResolution() const;

  //@{
  /**
   * Returns if minimum/maximum bound is set for the domain.
   */
  bool GetMinimumExists(size_t idx) const;
  bool GetMaximumExists(size_t idx) const;
  //@}

  /**
   * Returns the number of ranges defined.
   */
  size_t GetNumberOfRanges() const;

  enum DefaultModes
  {
    MIN,
    MAX,
    MID
  };

  /**
   * Get the default-mode that controls how SetDefaultValues() behaves.
   */
  DefaultModes GetDefaultMode(size_t index = 0);

  ///@{
  void Update() override;
  int IsInDomain(vtkSMProperty* property) const override;
  bool SetDefaultValues(vtkSMProperty*, bool use_unchecked_values) const override;
  ///@}

protected:
  vtkSMRangeDomainTemplate();
  ~vtkSMRangeDomainTemplate() override;

  ///@{
  bool ReadXMLAttributes(const pugi::xml_node& node) override;
  ///@}

private:
  vtkSMRangeDomainTemplate(const vtkSMRangeDomainTemplate&) = delete;
  void operator=(const vtkSMRangeDomainTemplate&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#include "vtkSMRangeDomainTemplate.txx"

#endif // !defined(vtkSMRangeDomainTemplate_h)
