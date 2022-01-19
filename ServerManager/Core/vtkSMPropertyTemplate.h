/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyTemplate.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMPropertyTemplate
 * @brief
 *
 */

#ifndef vtkSMPropertyTemplate_h
#define vtkSMPropertyTemplate_h

#include "vtkSMProperty.h"
#include "vtkServerManagerCoreModule.h" //needed for exports

#include <vector> // for std::vector

// The export macro below makes no sense, but is necessary for older compilers
// when we export instantiations of this class from this module
template <class ValueTypeT>
class VTKSERVERMANAGERCORE_EXPORT vtkSMPropertyTemplate : public vtkSMProperty
{
public:
  using SelfType = vtkSMPropertyTemplate<ValueTypeT>;
  vtkTemplateTypeMacro(SelfType, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * API to set the value of the property. Returns true if the value was
   * modified, else returns false.
   *
   * Setting value of a property automatically causes any checked values set to
   * be cleared. Note the unchecked values are cleared irrespective of whether
   * the new values set are different than the existing values.
   *
   * Note, these methods perform no checks for the value. Thus, it's possible to
   * set a property value in unacceptable ways. The application should use other
   * mechanisms to ensure property values are valid and acceptable.
   */
  bool SetElements(const std::vector<ValueTypeT>& elements);
  bool SetElements(const ValueTypeT* elements, size_t numElements);
  bool AppendElements(const std::vector<ValueTypeT>& elements);
  ///@}

  ///@{
  /**
   * Same as SetElements/AppendElements except for unchecked values of the
   * property.
   */
  bool SetUncheckedElements(const std::vector<ValueTypeT>& elements);
  bool SetUncheckedElements(const ValueTypeT* elements, size_t numElements);
  bool AppendUncheckedElements(const std::vector<ValueTypeT>& elements);
  ///@}

  // TODO:
  // /**
  //  * Set an element at a specific index. This will resize the property, if
  //  * needed.
  //  */
  // bool SetElement(size_t index, const ValueTypeT& value);

  // /**
  //  * Same as `SetElement` except for unchecked values of the property.
  //  */
  // bool SetUncheckedElement(size_t index, const ValueTypeT& value);

  /**
   * Returns the current value for the property.
   */
  const std::vector<ValueTypeT>& GetElements() const { return this->Elements; }

  /**
   * Returns the current unchecked value for the property.
   */
  const std::vector<ValueTypeT>& GetUncheckedElements() const
  {
    return this->UncheckedElementsValid ? this->UncheckedElements : this->Elements;
  }

  ///@{
  // Overridden from superclass.
  size_t GetNumberOfElements() const override { return this->Elements.size(); }
  size_t GetNumberOfUncheckedElements() const override
  {
    return this->UncheckedElementsValid ? this->UncheckedElements.size() : this->Elements.size();
  }
  void ClearUncheckedElements() override;
  void Copy(const vtkSMProperty* source) override;
  ///@}
protected:
  vtkSMPropertyTemplate();
  ~vtkSMPropertyTemplate() override;

  bool ReadXMLAttributes(const pugi::xml_node& node) override;
  vtkJson SaveState(bool verbose) const override;
  bool LoadState(const vtkJson& json, vtkSMStateContext* context) override;

  virtual ValueTypeT ToValue(const std::string& valueString) const = 0;
  virtual vtkJson ToJson(const ValueTypeT& value) const = 0;
  virtual ValueTypeT ToValue(const vtkJson& json, vtkSMStateContext* context) const = 0;

private:
  vtkSMPropertyTemplate(const vtkSMPropertyTemplate&) = delete;
  void operator=(const vtkSMPropertyTemplate&) = delete;

  std::vector<ValueTypeT> Elements;
  std::vector<ValueTypeT> UncheckedElements;
  std::vector<ValueTypeT> XMLDefaults;
  bool DefaultsValid{ false };
  bool ElementsValid{ false };
  bool UncheckedElementsValid{ false };
};

#include "vtkSMPropertyTemplate.txx"
#endif
