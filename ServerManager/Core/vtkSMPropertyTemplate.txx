/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyTemplate.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLogger.h"
#include "vtkJson.h"

#include <vtksys/SystemTools.hxx>
#include <algorithm>
#include <type_traits>

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkSMPropertyTemplate<ValueTypeT>::vtkSMPropertyTemplate() = default;

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkSMPropertyTemplate<ValueTypeT>::~vtkSMPropertyTemplate() = default;

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkSMPropertyTemplate<ValueTypeT>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkSMPropertyTemplate<ValueTypeT>::SetElements(const std::vector<ValueTypeT>& elements)
{
  const bool valuesChanged = (this->Elements != elements);
  const bool uncheckedValuesChanged = this->UncheckedElementsValid;
  this->UncheckedElements.clear();
  this->UncheckedElementsValid = false;
  this->ElementsValid = true;

  if (valuesChanged)
  {
    this->Elements = elements;
    this->Modified();
  }

  if (uncheckedValuesChanged || valuesChanged)
  {
    this->UncheckedPropertyModified();
  }
  return valuesChanged;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkSMPropertyTemplate<ValueTypeT>::SetElements(const ValueTypeT* elements, size_t numElements)
{
  std::vector<ValueTypeT> values;
  values.reserve(numElements);
  std::copy(elements, elements + numElements, std::back_inserter(values));
  return this->SetElements(values);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkSMPropertyTemplate<ValueTypeT>::AppendElements(const std::vector<ValueTypeT>& elements)
{
  this->UncheckedElements.clear();
  this->UncheckedElementsValid = false;

  this->Elements.reserve(this->Elements.size() + elements.size());
  std::copy(elements.begin(), elements.end(), std::back_inserter(this->Elements));
  this->ElementsValid = true;
  this->Modified();
  this->UncheckedPropertyModified();
  return true;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkSMPropertyTemplate<ValueTypeT>::SetUncheckedElements(const std::vector<ValueTypeT>& elements)
{
  const bool valuesChanged = !this->UncheckedElementsValid || (this->UncheckedElements != elements);
  if (valuesChanged)
  {
    this->UncheckedElements = elements;
    this->UncheckedElementsValid = true;
    this->UncheckedPropertyModified();
  }
  return valuesChanged;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkSMPropertyTemplate<ValueTypeT>::SetUncheckedElements(const ValueTypeT* elements, size_t numElements)
{
  std::vector<ValueTypeT> values;
  values.reserve(numElements);
  std::copy(elements, elements + numElements, std::back_inserter(values));
  return this->SetUncheckedElements(values);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkSMPropertyTemplate<ValueTypeT>::AppendUncheckedElements(const std::vector<ValueTypeT>& elements)
{
  this->UncheckedElements.reserve(this->UncheckedElements.size() + elements.size());
  std::copy(elements.begin(), elements.end(), std::back_inserter(this->UncheckedElements));
  this->UncheckedElementsValid = true;
  this->UncheckedPropertyModified();
  return true;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkSMPropertyTemplate<ValueTypeT>::ClearUncheckedElements()
{
  if (this->UncheckedElementsValid)
  {
    this->UncheckedElements.clear();
    this->UncheckedPropertyModified();
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkSMPropertyTemplate<ValueTypeT>::Copy(const vtkSMProperty* source)
{
  const auto* sourceP = vtkSMPropertyTemplate<ValueTypeT>::SafeDownCast(
      const_cast<vtkSMProperty*>(source));
  if (!sourceP)
  {
    vtkLogF(ERROR, "Copy not supported between properties on different types: %s and %s",
        this->GetClassName(), source->GetClassName());
    return;
  }

  const bool modified = this->Elements != sourceP->Elements ||
    this->ElementsValid != sourceP->ElementsValid;

  const bool uncheckedModified = this->UncheckedElements != sourceP->UncheckedElements ||
    this->UncheckedElementsValid != sourceP->UncheckedElementsValid;

  this->Elements = sourceP->Elements;
  this->ElementsValid = sourceP->ElementsValid;
  this->UncheckedElements= sourceP->UncheckedElements;
  this->UncheckedElementsValid = sourceP->UncheckedElementsValid;
  if (modified)
  {
    this->Modified();
  }
  if (modified || uncheckedModified)
  {
    this->UncheckedPropertyModified();
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkSMPropertyTemplate<ValueTypeT>::ReadXMLAttributes(const pugi::xml_node& node)
{
  if (!this->Superclass::ReadXMLAttributes(node))
  {
    return false;
  }

  const std::string delimiter = node.attribute("delimiter").as_string(" ");
  if (delimiter.size() != 1)
  {
    vtkLogF(ERROR, "'%s' is not a valid delimiter; must be a single character long.",
        delimiter.c_str());
    return false;
  }

  if (node.attribute("default_values"))
  {
    const auto defaults = vtksys::SystemTools::SplitString(
      node.attribute("default_values").as_string(), delimiter[0]);
    std::transform(defaults.begin(), defaults.end(),
        std::back_inserter(this->XMLDefaults),
        [this](const std::string& value) { return this->ToValue(value); });
    this->Elements = this->XMLDefaults;
    this->DefaultsValid = true;
    this->ElementsValid = true;
  }
  return true;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkJson vtkSMPropertyTemplate<ValueTypeT>::SaveState(bool verbose) const
{
  vtkJson node = this->Superclass::SaveState(verbose);
  node["type"] = this->GetElementType();
  if (this->ElementsValid)
  {
    auto array = vtkJson::array();
    for (auto& element : this->Elements)
    {
      array.push_back(this->ToJson(element));
    }
    node["elements"] = std::move(array);
  }

  return node;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkSMPropertyTemplate<ValueTypeT>::LoadState(const vtkJson& json, vtkSMStateContext* context)
{
  if (!this->Superclass::LoadState(json, context))
  {
    return false;
  }

  if (!json.contains("type") || json.at("type").get<int>() != this->GetElementType())
  {
    vtkLogF(ERROR, "missing 'type' information!");
    return false;
  }

  std::vector<ValueTypeT> elements;
  for (const auto& element : json.at("elements"))
  {
    elements.push_back(this->ToValue(element, context));
  }
  this->SetElements(elements);
  return true;
}
