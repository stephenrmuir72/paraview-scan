/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectStore.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectStore.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>

class vtkObjectStore::vtkInternals
{
public:
  std::mutex ObjectMapMutex;
  std::unordered_map<vtkTypeUInt32, vtkSmartPointer<vtkObject>> ObjectMap;
};

vtkStandardNewMacro(vtkObjectStore);
//----------------------------------------------------------------------------
vtkObjectStore::vtkObjectStore()
  : Internals(new vtkObjectStore::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkObjectStore::~vtkObjectStore()
{
  vtkLogScopeF(INFO, "delete object registry (%s)", vtkLogIdentifier(this));
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkObject> vtkObjectStore::FindObject(vtkTypeUInt32 gid) const
{
  auto& internals = (*this->Internals);
  const std::lock_guard<std::mutex> lk(internals.ObjectMapMutex);
  auto iter = internals.ObjectMap.find(gid);
  return iter != internals.ObjectMap.end() ? iter->second : nullptr;
}

//----------------------------------------------------------------------------
void vtkObjectStore::RegisterObject(vtkTypeUInt32 gid, vtkSmartPointer<vtkObject> const& object)
{
  if (object)
  {
    auto& internals = (*this->Internals);
    std::unique_lock<std::mutex> lk(internals.ObjectMapMutex);
    internals.ObjectMap[gid] = object;
    lk.unlock(); // release lock before firing signals
    this->Modified();
    this->InvokeEvent(vtkObjectStore::RegisterObjectEvent, object.GetPointer());
  }
}

//----------------------------------------------------------------------------
void vtkObjectStore::UnregisterObject(vtkTypeUInt32 gid)
{
  auto& internals = (*this->Internals);
  std::unique_lock<std::mutex> lk(internals.ObjectMapMutex);
  auto iter = internals.ObjectMap.find(gid);
  if (iter != internals.ObjectMap.end())
  {
    vtkSmartPointer<vtkObject> object = iter->second;
    internals.ObjectMap.erase(iter);
    lk.unlock(); // release lock before firing signals
    this->InvokeEvent(vtkObjectStore::UnregisterObjectEvent, iter->second.GetPointer());
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkObjectStore::GetID(const vtkObject* obj) const
{
  auto& internals = (*this->Internals);
  const std::lock_guard<std::mutex> lk(internals.ObjectMapMutex);
  auto iter = std::find_if(internals.ObjectMap.begin(), internals.ObjectMap.end(),
    [&obj](const auto& pair) { return pair.second.GetPointer() == obj; });
  return iter != internals.ObjectMap.end() ? iter->first : 0;
}

//----------------------------------------------------------------------------
void vtkObjectStore::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
