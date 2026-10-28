// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqThreadedUpdateBehavior.h"

#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqServer.h"
#include "vtkSMSession.h"
#include "vtkPVSessionCore.h"

#include <QEventLoop>

//-----------------------------------------------------------------------------
pqThreadedUpdateBehavior::pqThreadedUpdateBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerCreation(pqServer*)));
}

//-----------------------------------------------------------------------------
void pqThreadedUpdateBehavior::onServerCreation(pqServer* server)
{
  // Should use dedicated event instead of relying on random events
  // `Qt::QueuedConnection` matters here, that lets the VTKEvent/Qt Signal go through the thread barrier.
  this->VTKConnector->Connect(server->session()->GetSessionCore(), vtkCommand::TDxButtonPressEvent, this, SLOT(loop()));
  this->VTKConnector->Connect(server->session()->GetSessionCore(), vtkCommand::TDxButtonReleaseEvent, this, SIGNAL(endLoop()));
}

//-----------------------------------------------------------------------------
void pqThreadedUpdateBehavior::loop()
{
  QEventLoop qloop;
  QObject::connect(this, &pqThreadedUpdateBehavior::endLoop, &qloop, &QEventLoop::quit, Qt::QueuedConnection);
  qloop.exec();
}
