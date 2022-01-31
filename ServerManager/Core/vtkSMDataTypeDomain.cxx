/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataTypeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataTypeDomain.h"

#include "vtkDataObjectTypes.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <sstream>
#include <string>
#include <vector>

//*****************************************************************************
// Internal classes
//*****************************************************************************
struct vtkSMDataTypeDomainAllowedType
{
  int Type = -1;
  std::vector<int> ChildTypes;
  bool ChildMatchAny = false;
};

struct vtkSMDataTypeDomainInternals
{
  std::vector<vtkSMDataTypeDomainAllowedType> DataTypes;
};

//*****************************************************************************
vtkStandardNewMacro(vtkSMDataTypeDomain);
//---------------------------------------------------------------------------
vtkSMDataTypeDomain::vtkSMDataTypeDomain()
  : CompositeDataSupported(true)
  , CompositeDataRequired(false)
  , DTInternals(new vtkSMDataTypeDomainInternals())
{
}

//---------------------------------------------------------------------------
vtkSMDataTypeDomain::~vtkSMDataTypeDomain()
{
  delete this->DTInternals;
}

//---------------------------------------------------------------------------
int vtkSMDataTypeDomain::IsInDomain(vtkSMProperty* property) const
{
  if (this->GetIsOptional())
  {
    return IN_DOMAIN;
  }

  if (!property)
  {
    return NOT_APPLICABLE;
  }

  vtkSMPropertyHelper helper(property);
  helper.SetUseUnchecked(true);
  for (size_t cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
  {
    auto* proxy = helper.GetAsProxy(cc);
    int port = helper.GetOutputPort(cc);
    if (!this->IsInDomain(proxy, port))
    {
      return NOT_IN_DOMAIN;
    }
  }

  return IN_DOMAIN;
}

//---------------------------------------------------------------------------
int vtkSMDataTypeDomain::IsInDomain(vtkSMProxy* proxy, int outputport /*=0*/) const
{
  if (!proxy)
  {
    return 0;
  }

  if (this->DTInternals->DataTypes.empty())
  {
    return 1;
  }

  // Make sure the outputs are created.
  vtkPVDataInformation* info = proxy->GetDataInformation(outputport);
  if (!info || info->IsNull())
  {
    return 0;
  }

  if (info->IsCompositeDataSet() && !this->CompositeDataSupported)
  {
    return 0;
  }
  if (!info->IsCompositeDataSet() && this->CompositeDataRequired)
  {
    return 0;
  }

  for (auto& dtype : this->DTInternals->DataTypes)
  {
    if (info->DataSetTypeIsA(dtype.Type))
    {
      if (dtype.ChildTypes.empty())
      {
        return 1;
      }

      size_t match_count = 0;
      for (auto& childType : dtype.ChildTypes)
      {
        match_count += info->HasDataSetType(childType) ? 1 : 0;
      }

      if (dtype.ChildMatchAny && match_count > 0)
      {
        return 1;
      }
      else if (!dtype.ChildMatchAny && match_count == dtype.ChildTypes.size())
      {
        return 1;
      }
    }
  }

  return 0;
}

//---------------------------------------------------------------------------
std::string vtkSMDataTypeDomain::GetDomainDescription() const
{
  auto& types = this->DTInternals->DataTypes;
  if (types.empty())
  {
    return std::string{};
  }

  std::vector<std::string> phrases;
  for (auto& typeInfo : types)
  {
    if (typeInfo.ChildTypes.empty())
    {
      phrases.push_back(vtkDataObjectTypes::GetClassNameFromTypeId(typeInfo.Type));
    }
    else
    {
      std::ostringstream subPhrase;
      subPhrase << vtkDataObjectTypes::GetClassNameFromTypeId(typeInfo.Type);
      subPhrase << " containing";
      for (size_t cc = 0; cc < typeInfo.ChildTypes.size(); ++cc)
      {
        if (cc > 0)
        {
          subPhrase << ",";
          if (cc + 1 == typeInfo.ChildTypes.size())
          {
            subPhrase << (typeInfo.ChildMatchAny ? " or" : " and");
          }
        }
        subPhrase << " " << vtkDataObjectTypes::GetClassNameFromTypeId(typeInfo.ChildTypes[cc]);
      }
      phrases.push_back(subPhrase.str());
    }
  }

  std::ostringstream stream;
  stream << "Input must be ";
  for (size_t cc = 0; cc < phrases.size(); ++cc)
  {
    if (cc > 0)
    {
      stream << ", ";
      if (cc + 1 == phrases.size())
      {
        stream << "or ";
      }
    }
    stream << phrases[cc].c_str();
  }
  return stream.str();
}

//---------------------------------------------------------------------------
bool vtkSMDataTypeDomain::ReadXMLAttributes(const pugi::xml_node& node)
{
  if (!this->Superclass::ReadXMLAttributes(node))
  {
    return false;
  }

  this->CompositeDataSupported =
    node.attribute("composite_data_supported").as_bool(this->CompositeDataSupported);

  this->CompositeDataRequired =
    node.attribute("composite_data_required").as_bool(this->CompositeDataRequired);

  // Loop over the top-level elements.
  for (auto selement : node.children("DataType"))
  {
    const char* value = selement.attribute("value").as_string(nullptr);
    if (!value)
    {
      vtkErrorMacro(<< "Can not find required attribute 'value'");
      return false;
    }

    const int typeId = vtkDataObjectTypes::GetTypeIdFromClassName(value);
    if (typeId == -1)
    {
      vtkErrorMacro("'" << value << "' not a known data object type!");
      return false;
    }

    vtkSMDataTypeDomainAllowedType typeInfo;
    typeInfo.Type = typeId;

    const char* childMatch = selement.attribute("child_match").as_string("any");
    if (strcmp(childMatch, "any") != 0 && strcmp(childMatch, "all") != 0)
    {
      vtkErrorMacro("'child_match' can only be 'any' or 'all', got '" << childMatch << "'.");
      return false;
    }
    typeInfo.ChildMatchAny = (strcmp(childMatch, "any") == 0);
    for (auto childElement : selement.children("DataType"))
    {
      const char* childValue = childElement.attribute("value").as_string(nullptr);
      if (!childValue)
      {
        vtkErrorMacro("Missing required attribute 'value'. ");
        return false;
      }

      const auto childTypeId = vtkDataObjectTypes::GetTypeIdFromClassName(childValue);
      if (childTypeId == -1)
      {
        vtkErrorMacro("'" << childValue << "' not a known data object type!");
        return false;
      }

      typeInfo.ChildTypes.push_back(childTypeId);
    }

    this->DTInternals->DataTypes.push_back(typeInfo);
  }
  return true;
}

//---------------------------------------------------------------------------
void vtkSMDataTypeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
