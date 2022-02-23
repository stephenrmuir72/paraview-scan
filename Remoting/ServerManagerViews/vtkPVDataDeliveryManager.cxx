/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataDeliveryManager.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVLogger.h"
#include "vtkPVView.h"
#include "vtkService.h"
#include "vtkSmartPointer.h"
#include "vtkStringUtilities.h"
#include "vtkWeakPointer.h"

#include <algorithm>
#include <map>
#include <numeric>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

namespace
{
vtkSmartPointer<vtkDataObject> Clone(vtkDataObject* source)
{
  if (!source)
  {
    return nullptr;
  }
  auto clone = vtk::TakeSmartPointer(source->NewInstance());

  if (auto* cd = vtkCompositeDataSet::SafeDownCast(clone))
  {
    cd->RecursiveShallowCopy(source);
  }
  else
  {
    clone->ShallowCopy(source);
  }
  return clone;
}

struct vtkItem
{
  vtkSmartPointer<vtkPVDataRepresentation> Representation;

  vtkTimeStamp TimeStamp;
  vtkSmartPointer<vtkDataObject> Data;
  vtkSmartPointer<vtkInformation> MetaData{ vtkInformation::New() };

  vtkTimeStamp LODTimeStamp;
  vtkSmartPointer<vtkDataObject> LODData;
  vtkSmartPointer<vtkInformation> LODMetaData{ vtkInformation::New() };
};
}

class vtkPVDataDeliveryManager::vtkInternals
{
public:
  vtkPVView* View{ nullptr };
  std::map<vtkTypeUInt32, std::vector<vtkItem>> Items;

  const vtkItem& GetItem(vtkPVDataRepresentation* repr, int port) const
  {
    return this->Items.at(repr->GetGlobalID()).at(port);
  }

  vtkItem& GetItem(vtkPVDataRepresentation* repr, int port)
  {
    return this->Items.at(repr->GetGlobalID()).at(port);
  }

  const vtkItem& GetItem(vtkTypeUInt32 gid, int port) const { return this->Items.at(gid).at(port); }

  vtkItem& GetItem(vtkTypeUInt32 gid, int port)
  {
    auto& vector = this->Items[gid];
    if (port >= static_cast<int>(vector.size()))
    {
      vector.resize(port + 1);
    }
    return vector.at(port);
  }
};

//*****************************************************************************
vtkStandardNewMacro(vtkPVDataDeliveryManager);
vtkPVDataDeliveryManager::vtkPVDataDeliveryManager()
  : Internals(new vtkPVDataDeliveryManager::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVDataDeliveryManager::~vtkPVDataDeliveryManager() = default;

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetView(vtkPVView* view)
{
  auto& internals = (*this->Internals);
  internals.View = view;
}

//----------------------------------------------------------------------------
vtkPVView* vtkPVDataDeliveryManager::GetView() const
{
  const auto& internals = (*this->Internals);
  return internals.View;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::RegisterRepresentation(vtkPVDataRepresentation* repr)
{
  assert("A representation must have a valid GlobalID" && repr->GetGlobalID());
  auto& internals = (*this->Internals);
  // allocate space for all rendering-ports.
  auto& vector = internals.Items[repr->GetGlobalID()];
  vector.resize(repr->GetNumberOfRenderingPorts());
  for (auto& item : vector)
  {
    item.Representation = repr;
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::UnRegisterRepresentation(vtkPVDataRepresentation* repr)
{
  assert("A representation must have a valid GlobalID" && repr->GetGlobalID());

  auto& internals = (*this->Internals);
  const auto gid = repr->GetGlobalID();
  internals.Items.erase(gid);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetPiece(
  vtkPVDataRepresentation* repr, int port, vtkDataObject* data, bool useLOD)
{
  vtkLogF(TRACE, "SetPiece(repr=%s, port=%d, data=%p, useLOD=%d)", repr->GetLogName().c_str(), port,
    data, useLOD ? 1 : 0);

  auto& internals = (*this->Internals);
  auto& item = internals.GetItem(repr, port);
  item.TimeStamp.Modified();
  item.Data = ::Clone(data);

  // erase LOD to avoid using obsolete data.
  item.LODTimeStamp = vtkTimeStamp();
  item.LODData = nullptr;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkPVDataDeliveryManager::GetPiece(
  vtkPVDataRepresentation* repr, int port, bool useLOD)
{
  auto& internals = (*this->Internals);
  const auto& item = internals.GetItem(repr, port);
  return useLOD ? item.LODData : item.Data;
}

//----------------------------------------------------------------------------
bool vtkPVDataDeliveryManager::HasPiece(vtkPVDataRepresentation* repr, int port, bool useLOD)
{
  auto& internals = (*this->Internals);
  const auto& item = internals.GetItem(repr, port);
  auto& piece = useLOD ? item.LODData : item.Data;
  return piece != nullptr;
}

//----------------------------------------------------------------------------
vtkInformation* vtkPVDataDeliveryManager::GetInformation(
  vtkPVDataRepresentation* repr, int port, bool useLOD)
{
  auto& internals = (*this->Internals);
  const auto& item = internals.GetItem(repr, port);
  return useLOD ? item.LODMetaData : item.MetaData;
}

//----------------------------------------------------------------------------
vtkTypeUInt64 vtkPVDataDeliveryManager::GetVisibleDataSize(bool useLOD) const
{
  vtkTypeUInt64 size = 0;
  const auto& internals = (*this->Internals);
  for (const auto& pair : internals.Items)
  {
    const auto* repr =
      pair.second.empty() ? nullptr : pair.second.front().Representation.GetPointer();
    if (repr == nullptr || !repr->GetVisibility())
    {
      // skip hidden representations.
      continue;
    }

    size = std::accumulate(
      pair.second.begin(), pair.second.end(), size, [useLOD](vtkTypeUInt64 sum, const auto& item) {
        auto& data = useLOD && item.LODData ? item.LODData : item.Data;
        return sum + (data ? data->GetActualMemorySize() : 0);
      });
  }
  return size;
}

//----------------------------------------------------------------------------
std::map<std::string, vtkSmartPointer<vtkObject>> vtkPVDataDeliveryManager::GetData(
  vtkMTimeType timestamp) const
{
  const auto& internals = (*this->Internals);

  std::map<std::string, vtkSmartPointer<vtkObject>> dataMap;
  for (const auto& pair : internals.Items)
  {
    const auto* repr =
      pair.second.empty() ? nullptr : pair.second.front().Representation.GetPointer();
    if (repr == nullptr || !repr->GetVisibility())
    {
      // skip hidden representations.
      continue;
    }

    int port = 0;
    for (auto& item : pair.second)
    {
      if (item.TimeStamp > timestamp)
      {
        dataMap[fmt::format("{}:{}", repr->GetGlobalID(), port)] = item.Data;
      }
      ++port;
    }
  }

  return dataMap;
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::SetData(
  const std::map<std::string, vtkSmartPointer<vtkObject>>& dataMap)
{
  auto& internals = (*this->Internals);
  for (const auto& pair : dataMap)
  {
    std::vector<int> idx;
    vtkStringUtilities::Parse(pair.first, idx, ':');

    const auto gid = static_cast<vtkTypeUInt32>(idx[0]);
    const auto port = idx[1];

    auto& item = internals.GetItem(gid, port);
    item.TimeStamp.Modified();
    item.Data = vtkDataObject::SafeDownCast(pair.second);

    // erase LOD to avoid using obsolete data.
    item.LODTimeStamp = vtkTimeStamp();
    item.LODData = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
