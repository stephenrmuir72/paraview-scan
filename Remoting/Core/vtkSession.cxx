/*=========================================================================

  Program:   ParaView
  Module:    vtkSession.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSession.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProxyDefinitionManager.h"

class vtkSession::vtkInternals
{
public:
  vtkNew<vtkProxyDefinitionManager> ProxyDefinitionManager;
};

//----------------------------------------------------------------------------
vtkSession::vtkSession()
  : Internals(new vtkSession::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSession::~vtkSession() = default;

//----------------------------------------------------------------------------
vtkProxyDefinitionManager* vtkSession::GetProxyDefinitionManager() const
{
  return this->Internals->ProxyDefinitionManager;
}

//----------------------------------------------------------------------------
void vtkSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
