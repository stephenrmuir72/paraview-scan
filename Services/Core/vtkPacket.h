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
#include "vtkLogger.h"             // for vtkLogger.
#include "vtkServicesCoreModule.h" // for exports
#include "vtkSmartPointer.h"       // for vtkSmartPointer
#include "vtkUnsignedCharArray.h"  // for vtkUnsignedCharArray

#include <map>
#include <memory>
#include <string>

class vtkUnsignedCharArray;

class VTKSERVICESCORE_EXPORT vtkPacket
{
public:
  /**
   * Default constructor to create an empty packet.
   */
  vtkPacket();

  ///@{
  /**
   * Constructors to create a packet from a vtkJson object.
   */
  vtkPacket(const vtkJson& json);
  vtkPacket(vtkJson&& json);
  ///@}

  ///@{
  /**
   * Constructor overloads that enabling adding VTK-objects as payload.
   * Currently, only vtkUnsignedCharArray is supported if across wire exchange
   * is required.
   */
  vtkPacket(const vtkJson& json, const std::map<std::string, vtkSmartPointer<vtkObject>>& payload);
  vtkPacket(vtkJson&& json, const std::map<std::string, vtkSmartPointer<vtkObject>>& payload);
  ///@}

  ~vtkPacket() = default;
  vtkPacket(const vtkPacket&) = default;
  vtkPacket(vtkPacket&&) = default;
  vtkPacket& operator=(const vtkPacket&) = default;
  vtkPacket& operator=(vtkPacket&&) = default;

  /**
   * Provides access to internal Json.
   */
  const vtkJson& GetJSON() const;

  ///@{
  /**
   * Provides access to payload.
   */
  const std::map<std::string, vtkSmartPointer<vtkObject>> GetPayload() const
  {
    return this->Payload;
  }

  template <typename T>
  vtkSmartPointer<T> GetPayload(const std::string& key) const
  {
    auto iter = this->Payload.find(key);
    return iter != this->Payload.end() ? T::SafeDownCast(iter->second.GetPointer()) : nullptr;
  }

  ///@}

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
    archive & this->Payload.size();
    for (auto& pair : this->Payload)
    {
      archive& pair.first;
      auto* ucArray = vtkUnsignedCharArray::SafeDownCast(pair.second);
      vtkLogIfF(
        ERROR, ucArray == nullptr, "Currently, only vtkUnsignedCharArray can be serialized.");
      assert(ucArray != nullptr);
      archive & ucArray->GetNumberOfTuples();
      archive & ucArray->GetNumberOfComponents();
      archive.write(
        ucArray->GetPointer(0), ucArray->GetNumberOfComponents() * ucArray->GetNumberOfTuples());
    }
  }

  template <typename Ar>
  void load(Ar& archive)
  {
    this->Payload.clear();

    std::vector<uint8_t> data;
    archive& data;
    this->SetBinary(data);

    size_t count = 0;
    archive& count;
    for (size_t cc = 0; cc < count; ++cc)
    {
      std::string key;
      vtkIdType numTuples;
      int numComponents;

      archive& key;
      archive& numTuples;
      archive& numComponents;
      auto ucArray = vtk::TakeSmartPointer(vtkUnsignedCharArray::New());
      ucArray->SetNumberOfComponents(numComponents);
      ucArray->SetNumberOfTuples(numTuples);
      archive.read(ucArray->GetPointer(0), numTuples * numComponents);
      this->Payload[key] = ucArray;
    }
  }

  bool operator==(const vtkPacket& other) const { return *this == other.GetJSON(); }
  bool operator==(const vtkJson& json) const { return this->GetJSON() == json; }
  bool operator!=(const vtkPacket& other) const { return *this != other.GetJSON(); }
  bool operator!=(const vtkJson& json) const { return this->GetJSON() != json; }

private:
  std::shared_ptr<vtkJson> JSON;
  std::map<std::string, vtkSmartPointer<vtkObject>> Payload;

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
