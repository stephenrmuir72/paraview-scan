/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPacket.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPacket
 * @brief a data packet that can be passed around.
 *
 * vtkPacket represent a data packet. The remoting API is designed to move such
 * packets among services.
 *
 * Internally, vtkPacket stores a map of variants. The map can be accessed using
 * `vtkPacket::GetData`. This enables applications to store arbitrary data as
 * key-value pairs while preserving the type for the value. Only string-type keys
 * are supported.
 *
 * vtkPacket is immutable by design. Once created, its contents cannot be
 * modified.
 */

#ifndef vtkPacket_h
#define vtkPacket_h

#include "vtkJson.h"               // for vtkJson
#include "vtkServicesCoreModule.h" // for exports

#include <map>
#include <memory>
#include <string>

class VTKSERVICESCORE_EXPORT vtkPacket
{
public:
  vtkPacket();
  vtkPacket(const vtkJson& json);
  vtkPacket(vtkJson&& json);
  ~vtkPacket() = default;
  vtkPacket(const vtkPacket&) = default;
  vtkPacket(vtkPacket&&) = default;
  vtkPacket& operator=(const vtkPacket&) = default;
  vtkPacket& operator=(vtkPacket&&) = default;

  /**
   * Provides access to internal Json.
   */
  const vtkJson& GetJSON() const;

  /**
   * Print the contents of the packet. This is not not meant for serialization
   * but for printing for display or debugging.
   */
  void Print() const;
  void Print(std::ostream& ostream, int spaces) const;
  std::string ToString(int spaces = 2) const;

  template <typename Ar>
  void save(Ar& archive) const
  {
    archive & this->GetBinary();
  }

  template <typename Ar>
  void load(Ar& archive)
  {
    std::vector<uint8_t> data;
    archive& data;
    this->SetBinary(data);
  }

  bool operator==(const vtkPacket& other) const { return *this == other.GetJSON(); }
  bool operator==(const vtkJson& json) const { return this->GetJSON() == json; }

  bool operator!=(const vtkPacket& other) const { return *this != other.GetJSON(); }
  bool operator!=(const vtkJson& json) const { return this->GetJSON() != json; }

private:
  std::shared_ptr<vtkJson> JSON;

  ///@{
  /**
   * Serialize to/from CBOR format.
   */
  std::vector<uint8_t> GetBinary() const;
  void SetBinary(const std::vector<uint8_t>& buffer);
  ///@}
};

VTKSERVICESCORE_EXPORT void to_json(nlohmann::json& json, const vtkPacket& packet);
VTKSERVICESCORE_EXPORT void from_json(const nlohmann::json& json, vtkPacket& packet);

#endif
