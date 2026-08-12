// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVRendererCuller.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"

#include <set>

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRendererCuller);

//----------------------------------------------------------------------------
class vtkPVRendererCuller::vtkInternals
{
public:
  // List of props to never cull.
  std::set<void*> DoNotCullList;
};

//----------------------------------------------------------------------------
vtkPVRendererCuller::vtkPVRendererCuller()
  : Internals(std::make_unique<vtkInternals>())
{
}

//----------------------------------------------------------------------------
vtkPVRendererCuller::~vtkPVRendererCuller() = default;

//----------------------------------------------------------------------------
void vtkPVRendererCuller::AddToDoNotCullList(vtkProp* prop)
{
  this->Internals->DoNotCullList.insert(prop);
}

//----------------------------------------------------------------------------
void vtkPVRendererCuller::RemoveFromDoNotCullList(vtkProp* prop)
{
  this->Internals->DoNotCullList.erase(prop);
}

//----------------------------------------------------------------------------
double vtkPVRendererCuller::Cull(
  vtkRenderer* vtkNotUsed(ren), vtkProp** propList, int& listLength, int& initialized)
{
  const auto& doNotCullList = this->Internals->DoNotCullList;
  double total_time = 0;
  if (listLength <= 0)
  {
    return total_time;
  }
  double* allocatedTimeList = new double[listLength];
  for (int propLoop = 0; propLoop < listLength; ++propLoop)
  {
    // Get the prop out of the list
    vtkProp* prop = propList[propLoop];
    double prop_time = initialized ? prop->GetRenderTimeMultiplier() : 1.0;
    if (this->RenderOnLocalProcess == false && doNotCullList.find(prop) == doNotCullList.end())
    {
      prop_time = 0;
    }
    prop->SetRenderTimeMultiplier(prop_time);
    allocatedTimeList[propLoop] = prop_time;
    total_time += prop_time;
  }

  // ========================================================================
  // The following code is borrowed from vtkFrustumCoverageCuller.
  // ========================================================================
  // Now traverse the list from the beginning, swapping any zero entries back
  // in the list, while preserving the order of the non-zero entries. This
  // requires two indices for the two items we are comparing at any step.
  // The second index always moves back by one, but the first index moves back
  // by one only when it is pointing to something that has a non-zero value.
  int index1 = 0, index2 = 0;
  for (index2 = 1; index2 < listLength; index2++)
  {
    if (allocatedTimeList[index1] == 0.0)
    {
      if (allocatedTimeList[index2] != 0.0)
      {
        allocatedTimeList[index1] = allocatedTimeList[index2];
        propList[index1] = propList[index2];
        propList[index2] = nullptr;
        allocatedTimeList[index2] = 0.0;
      }
      else
      {
        propList[index1] = propList[index2] = nullptr;
        allocatedTimeList[index1] = allocatedTimeList[index2] = 0.0;
      }
    }
    if (allocatedTimeList[index1] != 0.0)
    {
      index1++;
    }
  }
  // Compute the new list length - index1 is always pointing to the
  // first 0.0 entry or the last entry if none were zero (in which case
  // we won't change the list length)
  listLength = (allocatedTimeList[index1] == 0.0) ? (index1) : listLength;
  delete[] allocatedTimeList;
  return total_time;
}

VTK_ABI_NAMESPACE_END
