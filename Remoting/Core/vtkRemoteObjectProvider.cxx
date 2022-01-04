/*=========================================================================

  Program:   ParaView
  Module:    vtkRemoteObjectProvider.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRemoteObjectProvider.h"

#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkProxyDefinitionManager.h"
#include "vtkSmartPointer.h"

class vtkRemoteObjectProvider::vtkInternals
{
public:
  vtkSmartPointer<vtkObjectStore> ObjectStore;
  vtkSmartPointer<vtkProxyDefinitionManager> ProxyDefinitionManager;
};

vtkStandardNewMacro(vtkRemoteObjectProvider);
//----------------------------------------------------------------------------
vtkRemoteObjectProvider::vtkRemoteObjectProvider()
  : Internals(new vtkRemoteObjectProvider::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkRemoteObjectProvider::~vtkRemoteObjectProvider() = default;

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::SetObjectStore(vtkObjectStore* store)
{
  auto& internals = (*this->Internals);
  internals.ObjectStore = store;
}

//----------------------------------------------------------------------------
vtkObjectStore* vtkRemoteObjectProvider::GetObjectStore() const
{
  const auto& internals = (*this->Internals);
  return internals.ObjectStore;
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::SetProxyDefinitionManager(vtkProxyDefinitionManager* mgr)
{
  auto& internals = (*this->Internals);
  internals.ProxyDefinitionManager = mgr;
}

//----------------------------------------------------------------------------
vtkProxyDefinitionManager* vtkRemoteObjectProvider::GetProxyDefinitionManager() const
{
  const auto& internals = (*this->Internals);
  return internals.ProxyDefinitionManager;
}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::InitializeInternal(vtkService* service) {}

//----------------------------------------------------------------------------
void vtkRemoteObjectProvider::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
