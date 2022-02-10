/*=========================================================================

   Program: ParaView
   Module:    pqProgressManager.cxx

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

========================================================================*/
#include "pqProgressManager.h"

#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkClientSession.h"
#include "vtkCommand.h"
#include "vtkPVGUIApplication.h"

#include <QPointer>
#include <QScopedValueRollback>

class pqProgressManager::pqInternals
{
public:
  QPointer<QObject> ObjectWithLock;
  int ProgressCount{ 0 };
  bool InProgressEvent{ false };
};

//-----------------------------------------------------------------------------
pqProgressManager::pqProgressManager(QObject* _parent)
  : QObject(_parent)
  , Internals(new pqProgressManager::pqInternals())
{
  auto* smmmodel = vtkPVGUIApplication::GetInstance()->GetServerManagerModel();

  // this is very basic right now, need to figure out how to handle progress
  // from different services and different session properly.
  QObject::connect(smmmodel, &pqServerManagerModel::serverAdded, [this](pqServer* server) {
    auto* session = server->session();
    session->GetProgressObservable().subscribe(rxcpp::util::apply_to(
      [this](const std::string& serviceName, const vtkRemoteObjectProvider::vtkProgressItem& item) {
        if (item.Progress == 100)
        {
          this->setProgress(QString::fromStdString(item.Message), item.Progress);
          this->setEnableProgress(false);
        }
        else if (item.Progress == 0)
        {
          this->setEnableProgress(true);
          this->setProgress(QString::fromStdString(item.Message), item.Progress);
        }
        else
        {
          this->setProgress(QString::fromStdString(item.Message), item.Progress);
        }
      }));
  });
}

//-----------------------------------------------------------------------------
pqProgressManager::~pqProgressManager() = default;

//-----------------------------------------------------------------------------
void pqProgressManager::lockProgress(QObject* object)
{
  auto& internals = (*this->Internals);
  if (internals.ObjectWithLock && internals.ObjectWithLock != object)
  {
    // already locked! cannot lock again.
    return;
  }

  if (!object)
  {
    return;
  }

  internals.ObjectWithLock = object;
}

//-----------------------------------------------------------------------------
void pqProgressManager::unlockProgress(QObject* object)
{
  auto& internals = (*this->Internals);
  if (internals.ObjectWithLock == object)
  {
    internals.ObjectWithLock = nullptr;
  }
}

//-----------------------------------------------------------------------------
bool pqProgressManager::isLocked() const
{
  const auto& internals = (*this->Internals);
  return (internals.ObjectWithLock != nullptr);
}

//-----------------------------------------------------------------------------
void pqProgressManager::setProgress(const QString& message, int progress_val)
{
  auto& internals = (*this->Internals);
  if (internals.ObjectWithLock && internals.ObjectWithLock != this->sender())
  {
    // When locked, ignore all other senders.
    return;
  }

  if (internals.InProgressEvent)
  {
    return;
  }

  QScopedValueRollback<bool> rollback(internals.InProgressEvent, true);
  Q_ASSERT(progress_val >= 0 && progress_val <= 100);
  Q_EMIT this->progress(message, progress_val);
}

//-----------------------------------------------------------------------------
void pqProgressManager::setEnableProgress(bool enable)
{
  auto& internals = (*this->Internals);
  if (internals.ObjectWithLock && internals.ObjectWithLock != this->sender())
  {
    // When locked, ignore all other senders.
    return;
  }

  if (enable)
  {
    if (internals.ProgressCount++ == 0)
    {
      Q_EMIT this->enableProgress(true);
    }
  }
  else
  {
    if (--internals.ProgressCount == 0)
    {
      Q_EMIT this->enableProgress(false);
    }
  }
}

//-----------------------------------------------------------------------------
void pqProgressManager::setEnableAbort(bool enable)
{
  auto& internals = (*this->Internals);
  if (internals.ObjectWithLock && internals.ObjectWithLock != this->sender())
  {
    // When locked, ignore all other senders.
    return;
  }
  Q_EMIT this->enableAbort(enable);
}

//-----------------------------------------------------------------------------
void pqProgressManager::triggerAbort()
{
  Q_EMIT this->abort();
}
