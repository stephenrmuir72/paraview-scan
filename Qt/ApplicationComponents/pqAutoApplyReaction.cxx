/*=========================================================================

   Program: ParaView
   Module:    pqAutoApplyReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqAutoApplyReaction.h"

#include "pqActiveObjects.h"
#include "pqServer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

//-----------------------------------------------------------------------------
pqAutoApplyReaction::pqAutoApplyReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  this->connect(
    &pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqAutoApplyReaction::onTriggered()
{
  pqAutoApplyReaction::setAutoApply(this->parentAction()->isChecked());
}

//-----------------------------------------------------------------------------
bool pqAutoApplyReaction::autoApply()
{
  auto* server = pqActiveObjects::instance().activeServer();
  if (auto* gsettings = server ? server->settingsProxy("GeneralSettings") : nullptr)
  {
    return vtkSMPropertyHelper(gsettings, "AutoApply").GetAsInt() == 1;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqAutoApplyReaction::setAutoApply(bool autoAccept)
{
  auto* server = pqActiveObjects::instance().activeServer();
  if (auto* gsettings = server ? server->settingsProxy("GeneralSettings") : nullptr)
  {
    vtkSMPropertyHelper(gsettings, "AutoApply").Set(autoAccept ? 1 : 0);
    gsettings->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
void pqAutoApplyReaction::updateEnableState()
{
  auto* server = pqActiveObjects::instance().activeServer();
  const bool enabled = server != nullptr;
  const bool checked = server ? this->autoApply() : false;
  auto* actn = this->parentAction();
  actn->setEnabled(enabled);
  actn->setChecked(checked);
  if (server)
  {
    QObject::connect(server, SIGNAL(settingsProxyChanged()), this, SLOT(updateEnableState()),
      Qt::UniqueConnection);
  }
}
