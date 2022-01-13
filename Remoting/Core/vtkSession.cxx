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

// TODO: this will need some cleanup
#include "paraview_server_manager.h"

class vtkSession::vtkInternals
{
public:
  vtkNew<vtkProxyDefinitionManager> ProxyDefinitionManager;
};

//----------------------------------------------------------------------------
vtkSession::vtkSession()
  : Internals(new vtkSession::vtkInternals())
{
  const auto& internals = (*this->Internals);

  // TODO: load this only for vtkServerSession and for client session, sync them
  // with the server session during handshake.
  std::vector<std::string> xmls;
  paraview_server_manager_initialize(xmls);
  for (const auto& xml : xmls)
  {
    internals.ProxyDefinitionManager->LoadConfigurationXML(xml.c_str());
  }
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
