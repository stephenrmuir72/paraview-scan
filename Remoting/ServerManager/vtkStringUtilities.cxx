/*=========================================================================

  Program:   ParaView
  Module:    vtkStringUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStringUtilities.h"

#include "vtkObjectFactory.h"
#include <string>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkStringUtilities);
//----------------------------------------------------------------------------
vtkStringUtilities::vtkStringUtilities() = default;

//----------------------------------------------------------------------------
vtkStringUtilities::~vtkStringUtilities() = default;

//----------------------------------------------------------------------------
bool vtkStringUtilities::EndsWith(const std::string& text, const std::string& suffix)
{
  if (text.size() < suffix.size())
  {
    return false;
  }

  return text.substr(text.size() - suffix.size()) == suffix;
}

//----------------------------------------------------------------------------
std::string vtkStringUtilities::GetPrettyLabel(const std::string& txt)
{
  return vtksys::SystemTools::AddSpaceBetweenCapitalizedWords(txt);
}

//----------------------------------------------------------------------------
bool vtkStringUtilities::Parse(const std::string& value, double& result)
{
  try
  {
    result = std::stod(value);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

//----------------------------------------------------------------------------
bool vtkStringUtilities::Parse(const std::string& value, int& result)
{
  try
  {
    result = std::stoi(value);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

//----------------------------------------------------------------------------
bool vtkStringUtilities::Parse(const std::string& value, vtkIdType& result)
{
  try
  {
    result = static_cast<vtkIdType>(std::stoll(value));
    return true;
  }
  catch (...)
  {
    return false;
  }
}

//----------------------------------------------------------------------------
void vtkStringUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
