/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPacket.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPacket.h"

#include "vtkObjectFactory.h"

#include <sstream>

#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/core.h)
// clang-format on

//----------------------------------------------------------------------------
vtkPacket::vtkPacket()
  : JSON(std::make_shared<vtkJson>())
{
}

//----------------------------------------------------------------------------
vtkPacket::vtkPacket(const vtkJson& json)
  : JSON(std::make_shared<vtkJson>(json))
{
}

//----------------------------------------------------------------------------
vtkPacket::vtkPacket(vtkJson&& json)
  : JSON(std::make_shared<vtkJson>(std::move(json)))
{
}

//----------------------------------------------------------------------------
const vtkJson& vtkPacket::GetJSON() const
{
  return *this->JSON;
}

//----------------------------------------------------------------------------
void vtkPacket::Print() const
{
  this->Print(cout, 2);
}

//----------------------------------------------------------------------------
void vtkPacket::Print(std::ostream& ostream, int spaces) const
{
  ostream << this->JSON->dump(spaces) << endl;
}

//----------------------------------------------------------------------------
std::string vtkPacket::ToString(int spaces) const
{
  return this->JSON->dump(spaces);
}

//----------------------------------------------------------------------------
std::vector<uint8_t> vtkPacket::GetBinary() const
{
  return vtkJson::to_cbor(*this->JSON);
}

//----------------------------------------------------------------------------
void vtkPacket::SetBinary(const std::vector<uint8_t>& buffer)
{
  *this->JSON = vtkJson::from_cbor(buffer);
}

//----------------------------------------------------------------------------
void to_json(nlohmann::json& json, const vtkPacket& packet)
{
  json = packet.GetJSON();
}

//----------------------------------------------------------------------------
void from_json(const nlohmann::json& json, vtkPacket& packet)
{
  packet = vtkPacket{ json };
}
