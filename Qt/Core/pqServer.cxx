/*=========================================================================

   Program: ParaView
   Module:    pqServer.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqServer.h"

#include "vtkSmartPointer.h"
// #include "vtkPVServerInformation.h"
#include "pqServerResource.h"
#include "vtkClientSession.h"
#include "vtkPVApplication.h"
#include "vtkSMSessionProxyManager.h"

// Qt includes.
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QStringList>
#include <QTimer>
#include <QtDebug>

class pqServer::pqInternals
{
public:
  vtkSmartPointer<vtkClientSession> Session;
  vtkTypeUInt32 SessionID;
  pqServerResource Resource;
};

//-----------------------------------------------------------------------------
pqServer::pqServer(vtkClientSession* session, QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqServer::pqInternals())
{
  auto& internals = *(this->Internals);
  internals.Session = session;
  internals.SessionID = vtkPVApplication::GetInstance()->GetSessionID(session);

  // FIXME: ASYNC
  // how do we monitor when the connection termintes
  // this->Internals->VTKConnect->Connect(this->Session, vtkPVSessionBase::ConnectionLost, this,
  //  SLOT(onConnectionLost(vtkObject*, ulong, void*, void*)));
}

//-----------------------------------------------------------------------------
pqServer::~pqServer() = default;

//-----------------------------------------------------------------------------
vtkClientSession* pqServer::session() const
{
  const auto& internals = (*this->Internals);
  return internals.Session;
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 pqServer::sessionID() const
{
  const auto& internals = (*this->Internals);
  return internals.SessionID;
}

//-----------------------------------------------------------------------------
void pqServer::setResource(const pqServerResource& server_resource)
{
  auto& internals = (*this->Internals);
  internals.Resource = server_resource;
  Q_EMIT this->nameChanged(this);
}

//-----------------------------------------------------------------------------
const pqServerResource& pqServer::resource() const
{
  const auto& internals = (*this->Internals);
  return internals.Resource;
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel* pqServer::activeSourcesSelectionModel() const
{
  vtkSMSessionProxyManager* pxm = this->proxyManager();
  return pxm->GetSelectionModel("ActiveSources");
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel* pqServer::activeViewSelectionModel() const
{
  vtkSMSessionProxyManager* pxm = this->proxyManager();
  return pxm->GetSelectionModel("ActiveView");
}

//-----------------------------------------------------------------------------
int pqServer::numberOfRanks() const
{
  // FIXME: ASYNC
  // return this->Session->GetNumberOfProcesses(
  //  vtkPVSession::DATA_SERVER | vtkPVSession::RENDER_SERVER);
  return 1;
}

//-----------------------------------------------------------------------------
bool pqServer::isRemote() const
{
  const auto& internals = (*this->Internals);
  return internals.Session->GetIsRemote();
}

//-----------------------------------------------------------------------------
vtkPVServerInformation* pqServer::serverInformation() const
{
  // FIXME: ASYNC
  // return this->Session->GetServerInformation();
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkSMSessionProxyManager* pqServer::proxyManager() const
{
  const auto& internals = (*this->Internals);
  return internals.Session->GetProxyManager();
}

//-----------------------------------------------------------------------------
void pqServer::onConnectionLost(vtkObject*, unsigned long, void*, void*)
{
  Q_EMIT serverSideDisconnected();
}
