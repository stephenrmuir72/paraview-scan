/*=========================================================================

  Program:   ParaView
  Module:    vtkSMEnumerationDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMEnumerationDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"

#include <algorithm>
#include <sstream>

namespace
{
struct EntryType
{
  std::string Text;
  int Value;
  std::string Info;

  EntryType()
    : Value(-1)
  {
  }
  EntryType(const char* text, int val, const char* info)
    : Value(val)
  {
    if (text)
    {
      this->Text = text;
    }
    if (info)
    {
      this->Info = info;
    }
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EntryType, Text, Value, Info);
};
}

struct vtkSMEnumerationDomain::vtkEDInternals
{
  using EntriesType = std::vector<EntryType>;
  EntriesType Entries;
};

vtkStandardNewMacro(vtkSMEnumerationDomain);
//---------------------------------------------------------------------------
vtkSMEnumerationDomain::vtkSMEnumerationDomain()
  : EInternals(new vtkSMEnumerationDomain::vtkEDInternals())
{
}

//---------------------------------------------------------------------------
vtkSMEnumerationDomain::~vtkSMEnumerationDomain() = default;

//---------------------------------------------------------------------------
unsigned int vtkSMEnumerationDomain::GetNumberOfEntries() const
{
  return static_cast<unsigned int>(this->EInternals->Entries.size());
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::GetEntryValue(unsigned int idx) const
{
  if (idx >= static_cast<unsigned int>(this->EInternals->Entries.size()))
  {
    vtkErrorMacro("Invalid idx: " << idx);
    return 0;
  }
  return this->EInternals->Entries[idx].Value;
}

//---------------------------------------------------------------------------
const char* vtkSMEnumerationDomain::GetEntryText(unsigned int idx) const
{
  if (idx >= static_cast<unsigned int>(this->EInternals->Entries.size()))
  {
    vtkErrorMacro("Invalid idx: " << idx);
    return nullptr;
  }
  return this->EInternals->Entries[idx].Text.c_str();
}

//---------------------------------------------------------------------------
const char* vtkSMEnumerationDomain::GetInfoText(unsigned int idx) const
{
  if (idx >= static_cast<unsigned int>(this->EInternals->Entries.size()))
  {
    vtkErrorMacro("Invalid idx: " << idx);
    return nullptr;
  }
  const auto& item = this->EInternals->Entries[idx];
  return item.Info.empty() ? nullptr : item.Info.c_str();
}

//---------------------------------------------------------------------------
const char* vtkSMEnumerationDomain::GetEntryTextForValue(int value) const
{
  unsigned int idx = 0;
  if (!this->IsInDomain(value, idx))
  {
    return nullptr;
  }
  return this->GetEntryText(idx);
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::HasEntryText(const char* text) const
{
  int valid;
  this->GetEntryValue(text, valid);
  return valid;
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::GetEntryValueForText(const char* text) const
{
  int valid;
  return this->GetEntryValue(text, valid);
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::GetEntryValue(const char* text, int& valid) const
{
  valid = 0;
  if (!text)
  {
    return -1;
  }

  const std::string str_text(text);
  const auto& entries = this->EInternals->Entries;
  auto iter = std::find_if(
    entries.begin(), entries.end(), [&](const EntryType& item) { return item.Text == str_text; });
  if (iter != entries.end())
  {
    valid = 1;
    return iter->Value;
  }

  return -1;
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::IsInDomain(vtkSMProperty* property) const
{
  if (this->GetIsOptional())
  {
    return IN_DOMAIN;
  }

  if (!property)
  {
    return NOT_APPLICABLE;
  }

  vtkSMIntVectorProperty* ip = vtkSMIntVectorProperty::SafeDownCast(property);
  if (ip)
  {
    for (const auto& elem : ip->GetUncheckedElements())
    {
      unsigned int idx;
      if (!this->IsInDomain(elem, idx))
      {
        return NOT_IN_DOMAIN;
      }
    }
    return IN_DOMAIN;
  }

  return NOT_IN_DOMAIN;
}

//---------------------------------------------------------------------------
int vtkSMEnumerationDomain::IsInDomain(int val, unsigned int& idx) const
{
  unsigned int numEntries = this->GetNumberOfEntries();
  if (numEntries == 0)
  {
    return 0;
  }

  const auto& entries = this->EInternals->Entries;
  auto iter = std::find_if(
    entries.begin(), entries.end(), [&](const EntryType& item) { return item.Value == val; });
  if (iter != entries.end())
  {
    idx = static_cast<int>(iter - entries.begin());
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
vtkJson vtkSMEnumerationDomain::SaveState(bool verbose) const
{
  auto json = this->Superclass::SaveState(verbose);
  if (verbose)
  {
    json["entries"] = this->EInternals->Entries;
  }
  return json;
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::AddEntry(const char* text, int value, const char* info /*=nullptr*/)
{
  this->EInternals->Entries.push_back(EntryType(text, value, info));
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::RemoveAllEntries()
{
  this->EInternals->Entries.erase(
    this->EInternals->Entries.begin(), this->EInternals->Entries.end());
  this->Modified();
}

//---------------------------------------------------------------------------
bool vtkSMEnumerationDomain::ReadXMLAttributes(const pugi::xml_node& node)
{
  if (!this->Superclass::ReadXMLAttributes(node))
  {
    return false;
  }

  // Loop over the top-level elements.
  for (auto child : node.children("Entry"))
  {
    if (!child.attribute("text") || !child.attribute("value"))
    {
      vtkErrorMacro("Invalid XML. Missing 'text' or 'value'.");
      return false;
    }

    const char* text = child.attribute("text").as_string();
    int value = child.attribute("value").as_int();
    this->AddEntry(text, value, child.attribute("info").as_string(nullptr));
  }

  return true;
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::Update()
{
#if 0 // FIXME: ASYNC
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (ivp && ivp->GetInformationOnly())
  {
    this->RemoveAllEntries();
    unsigned int max = ivp->GetNumberOfElements();
    for (unsigned int cc = 0; cc < max; ++cc)
    {
      std::ostringstream stream;
      stream << ivp->GetElement(cc);
      this->AddEntry(stream.str().c_str(), ivp->GetElement(cc));
    }
    this->InvokeModified();
  }
#endif
}

//---------------------------------------------------------------------------
bool vtkSMEnumerationDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) const
{
#if 0 // FIXME: ASYNC
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (ivp && this->GetNumberOfEntries() > 0)
  {
    unsigned int idx = 0;
    if (!this->IsInDomain(ivp->GetDefaultValue(0), idx))
    {
      if (use_unchecked_values)
      {
        ivp->SetUncheckedElement(0, this->GetEntryValue(0));
      }
      else
      {
        ivp->SetElement(0, this->GetEntryValue(0));
      }
      return true;
    }
  }
#endif
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//---------------------------------------------------------------------------
void vtkSMEnumerationDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
