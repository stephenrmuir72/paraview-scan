/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSessionProxyManagerInternals.h"

#include <cassert>

struct vtkSMProxyIteratorInternals
{
  vtkWeakPointer<vtkSMSessionProxyManager> ProxyManager;

  using ProxiesType = decltype(vtkSMSessionProxyManagerInternals::RegisteredProxies);
  using IteratorType = ProxiesType::iterator;
  IteratorType Iterator;

  IteratorType NextUntil(
    IteratorType iter, IteratorType end, std::function<bool(const std::string&)> criteria) const
  {
    while (iter != end && !criteria(iter->first.first))
    {
      ++iter;
    }

    return iter;
  }
};

vtkStandardNewMacro(vtkSMProxyIterator);
//---------------------------------------------------------------------------
vtkSMProxyIterator::vtkSMProxyIterator()
{
  this->Internals = new vtkSMProxyIteratorInternals;
  this->Mode = vtkSMProxyIterator::ALL;
}

//---------------------------------------------------------------------------
vtkSMProxyIterator::~vtkSMProxyIterator()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::SetSessionProxyManager(vtkSMSessionProxyManager* pxm)
{
  this->Internals->ProxyManager = pxm;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Begin(const char* groupName)
{
  auto& internals = (*this->Internals);
  vtkSMSessionProxyManager* pm = internals.ProxyManager;
  if (!pm)
  {
    vtkWarningMacro("ProxyManager is not set. Can not perform operation: Begin()");
    return;
  }
  this->SetModeToOneGroup();

  auto& registeredProxies = pm->Internals->RegisteredProxies;
  internals.Iterator = internals.NextUntil(registeredProxies.begin(), registeredProxies.end(),
    [&](const std::string& group) { return group == groupName; });
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Begin()
{
  auto& internals = (*this->Internals);
  vtkSMSessionProxyManager* pm = internals.ProxyManager;
  if (!pm)
  {
    vtkWarningMacro("ProxyManager is not set. Can not perform operation: Begin()");
    return;
  }

  auto& registeredProxies = pm->Internals->RegisteredProxies;
  internals.Iterator = registeredProxies.begin();
}

//---------------------------------------------------------------------------
int vtkSMProxyIterator::IsAtEnd()
{
  auto& internals = (*this->Internals);
  vtkSMSessionProxyManager* pm = internals.ProxyManager;
  if (pm == nullptr)
  {
    return 1;
  }

  auto& registeredProxies = pm->Internals->RegisteredProxies;
  return internals.Iterator == registeredProxies.end() ? 1 : 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::Next()
{
  auto& internals = (*this->Internals);
  vtkSMSessionProxyManager* pm = internals.ProxyManager;
  if (!pm)
  {
    vtkWarningMacro("ProxyManager is not set. Can not perform operation: Begin()");
    return;
  }

  assert(!this->IsAtEnd());

  auto& registeredProxies = pm->Internals->RegisteredProxies;
  const std::string currentGroup = this->GetGroup();

  internals.Iterator = internals.NextUntil(
    std::next(internals.Iterator), registeredProxies.end(), [&](const std::string& group) {
      switch (this->Mode)
      {
        case GROUPS_ONLY:
          return group != currentGroup;
        case ONE_GROUP:
          return group == currentGroup;
        default:
          return true;
      }
    });
}

//---------------------------------------------------------------------------
const char* vtkSMProxyIterator::GetGroup()
{
  const auto& internals = (*this->Internals);
  assert(!this->IsAtEnd());
  return internals.Iterator->first.first.c_str();
}

//---------------------------------------------------------------------------
const char* vtkSMProxyIterator::GetKey()
{
  const auto& internals = (*this->Internals);
  assert(!this->IsAtEnd());
  return internals.Iterator->first.second.c_str();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyIterator::GetProxy()
{
  const auto& internals = (*this->Internals);
  assert(!this->IsAtEnd());
  return internals.Iterator->second.GetPointer();
}

//---------------------------------------------------------------------------
void vtkSMProxyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << this->Mode << endl;
}
