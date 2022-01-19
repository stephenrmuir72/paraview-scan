/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRangeDomainTemplate.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRangeDomainTemplate.h"

#include "vtkSMPropertyTemplate.h"
#include "vtkStringUtilities.h"

#include <cassert>
#include <vtksys/SystemTools.hxx>

template <typename T>
struct Item
{
  T Value[2];
  bool Valid[2] = {false, false};
  int DefaultMode = vtkSMRangeDomainTemplate<T>::MID;

  bool operator==(const Item<T>& other) const
  {
    return (this->Value[0] == other.Value[0] &&
        this->Value[1] == other.Value[1] &&
        this->Valid[0] == other.Valid[0] &&
        this->Valid[1] == other.Valid[1] &&
        this->DefaultMode == other.DefaultMode);
  }
};

template <class T>
class vtkSMRangeDomainTemplate<T>::vtkInternals
{
public:
  std::vector<Item<T>> Ranges;
  int Resolution = -1;

  T ComputeDefault(size_t index) const
  {
    index = index % this->Ranges.size();
    const auto& item = this->Ranges[index];
    if (item.Valid[0] && item.Valid[1])
    {
      switch (item.DefaultMode)
      {
        case vtkSMRangeDomainTemplate<T>::MIN:
          return item.Value[0];
        case vtkSMRangeDomainTemplate<T>::MAX:
          return item.Value[1];
        default:
          return (item.Value[0] + item.Value[1]) / 2;
      }
    }
    if (item.Valid[0])
    {
      return item.Value[0];
    }

    return item.Valid[1] ? item.Value[1] : T{};
  }
};

//-----------------------------------------------------------------------------
template <class T>
vtkSMRangeDomainTemplate<T>::vtkSMRangeDomainTemplate()
  : Internals(new vtkSMRangeDomainTemplate<T>::vtkInternals())
{
}

//-----------------------------------------------------------------------------
template <class T>
vtkSMRangeDomainTemplate<T>::~vtkSMRangeDomainTemplate() = default;

//-----------------------------------------------------------------------------
template <class T>
int vtkSMRangeDomainTemplate<T>::IsInDomain(vtkSMProperty* property) const
{
  auto* tProperty = vtkSMPropertyTemplate<T>::SafeDownCast(property);
  if (!tProperty)
  {
    return NOT_APPLICABLE;
  }

  auto& elements = tProperty->GetElements();
  for (size_t cc=0; cc < elements.size(); ++cc)
  {
    if (!this->IsInDomain(cc, elements[cc]))
    {
      return NOT_IN_DOMAIN;
    }
  }

  return IN_DOMAIN;
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::IsInDomain(size_t idx, T val) const
{
  const auto numRanges = this->GetNumberOfRanges();
  if (numRanges == 0)
  {
    // no ranges specified, value is indeed in domain!
    return true;
  }
  idx = idx % numRanges;
  const auto& internals = (*this->Internals);
  const auto& range = internals.Ranges.at(idx);
  return (!range.Valid[0] || range.Value[0] <= val) && (!range.Value[1] || range.Value[1] >= val);
}

//-----------------------------------------------------------------------------
template <class T>
T vtkSMRangeDomainTemplate<T>::GetMinimum(size_t idx, bool* exists) const
{
  const auto numRanges = this->GetNumberOfRanges();
  if (numRanges == 0)
  {
    // no ranges specified, value is indeed in domain!
    if (exists) { *exists = false; }
    return {};
  }

  idx = idx % numRanges;
  const auto& internals = (*this->Internals);
  const auto& range = internals.Ranges.at(idx);
  if (exists) { *exists = range.Valid[0]; }
  return range.Valid[0] ? range.Value[0] : T{};
}

//-----------------------------------------------------------------------------
template <class T>
T vtkSMRangeDomainTemplate<T>::GetMaximum(size_t idx, bool* exists) const
{
  const auto numRanges = this->GetNumberOfRanges();
  if (numRanges == 0)
  {
    // no ranges specified, value is indeed in domain!
    if (exists) { *exists = false; }
    return {};
  }

  idx = idx % numRanges;
  const auto& internals = (*this->Internals);
  const auto& range = internals.Ranges.at(idx);
  if (exists) { *exists = range.Valid[1]; }
  return range.Valid[1] ? range.Value[1] : T{};
}

//-----------------------------------------------------------------------------
template <class T>
int vtkSMRangeDomainTemplate<T>::GetResolution() const
{
  return this->Resolution;
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::GetMinimumExists(size_t idx) const
{
  const auto numRanges = this->GetNumberOfRanges();
  if (numRanges == 0)
  {
    // no ranges specified, value is indeed in domain!
    return false;
  }

  idx = idx % numRanges;
  const auto& internals = (*this->Internals);
  const auto& range = internals.Ranges.at(idx);
  return range.Value[0];
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::GetMaximumExists(size_t idx) const
{
  const auto numRanges = this->GetNumberOfRanges();
  if (numRanges == 0)
  {
    // no ranges specified, value is indeed in domain!
    return false;
  }

  idx = idx % numRanges;
  const auto& internals = (*this->Internals);
  const auto& range = internals.Ranges.at(idx);
  return range.Value[1];
}

//-----------------------------------------------------------------------------
template <class T>
size_t vtkSMRangeDomainTemplate<T>::GetNumberOfRanges() const
{
  const auto& internals = (*this->Internals);
  return internals.Ranges.size();
}

//-----------------------------------------------------------------------------
template <class T>
void vtkSMRangeDomainTemplate<T>::Update()
{
  auto* tProperty = vtkSMPropertyTemplate<T>::SafeDownCast(this->GetRequiredProperty("Range"));
  if (!tProperty)
  {
    return;
  }

  const auto& elements = tProperty->GetUncheckedElements();
  auto& internals = (*this->Internals);
  std::vector<Item<T>> ranges = internals.Ranges;
  for (size_t cc=0; (cc+1) < elements.size(); cc+=2)
  {
    size_t index = cc / 2;
    if (ranges.size() <= index)
    {
      ranges.resize(index+1);
    }

    auto& item = ranges[index];
    item.Valid[0] = item.Valid[1] = true;
    item.Value[0] = elements[cc];
    item.Value[1] = elements[cc+1];
  }

  if (internals.Ranges != ranges)
  {
    internals.Ranges = std::move(ranges);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::ReadXMLAttributes(const pugi::xml_node& node)
{
  if (!this->Superclass::ReadXMLAttributes(node))
  {
    return false;
  }

  auto& internals = (*this->Internals);

  auto minValues = vtksys::SystemTools::SplitString(
      node.attribute("min").as_string(), ' ');
  auto maxValues = vtksys::SystemTools::SplitString(
      node.attribute("max").as_string(), ' ');
  auto defaultModes = vtksys::SystemTools::SplitString(
      node.attribute("default_mode").as_string(), ' ');

  const size_t count = std::max(std::max(minValues.size(), maxValues.size()), defaultModes.size());
  internals.Ranges.resize(count);

  for (size_t cc = 0; cc < count; ++cc)
  {
    auto& item = internals.Ranges[cc];
    if ((item.Valid[0] = cc < minValues.size()))
    {
      item.Valid[0] = vtkStringUtilities::Parse(minValues[cc], item.Value[0]);
    }
    if ((item.Valid[1] = cc < maxValues.size()))
    {
      item.Valid[1] = vtkStringUtilities::Parse(maxValues[cc], item.Value[1]);
    }

    if (!defaultModes.empty())
    {
      auto mode = cc < defaultModes.size() ?
        defaultModes[cc] : defaultModes.front();
      if (mode == "min")
      {
        item.DefaultMode = MIN;
      }
      else if (mode == "max")
      {
        item.DefaultMode = MAX;
      }
      else
      {
        item.DefaultMode = MID;
      }
    }
  }

  internals.Resolution = node.attribute("resolution").as_int(-1);
  return true;
}

//-----------------------------------------------------------------------------
template <class T>
bool vtkSMRangeDomainTemplate<T>::SetDefaultValues(
  vtkSMProperty* property, bool useUnchecked) const
{
  const auto& internals = (*this->Internals);
  auto tProperty = vtkSMPropertyTemplate<T>::SafeDownCast(property);
  if (!tProperty)
  {
    vtkLogF(ERROR, "Type mismatch!");
    return false;
  }

  if (this->GetNumberOfRequiredProperties() == 0)
  {
    // no required properties, the domain doesn't depend on run-time values.
    // Typically implies that the default values set for the property are what
    // the user intended.
    return false;
  }

  if (internals.Ranges.empty())
  {
    return false;
  }

  std::vector<T> values;
  values.resize(tProperty->GetNumberOfComponents());
  for (size_t cc=0; cc < tProperty->GetNumberOfComponents(); ++cc)
  {
    values[cc] = internals.ComputeDefault(cc);
  }

  if (useUnchecked)
  {
    tProperty->SetUncheckedElements(values);
  }
  else
  {
    tProperty->SetElements(values);
  }

  return true;
}

//-----------------------------------------------------------------------------
template <class T>
typename vtkSMRangeDomainTemplate<T>::DefaultModes vtkSMRangeDomainTemplate<T>::GetDefaultMode(
  size_t index)
{
  const auto& internals = (*this->Internals);
  if (internals.Ranges.empty())
  {
    return MID;
  }

  index = index % internals.Ranges.size();
  return internals.Ranges[index].DefaultMode;
}

//-----------------------------------------------------------------------------
template <class T>
void vtkSMRangeDomainTemplate<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
