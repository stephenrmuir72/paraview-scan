/*=========================================================================

   Program: ParaView
   Module:    pqMainControlsToolbar.cxx

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
#include "pqMainControlsToolbar.h"
#include "ui_pqMainControlsToolbar.h"

#include "pqAutoApplyReaction.h"
#include "pqCatalystExportReaction.h"
// #include "pqDataQueryReaction.h"
// #include "pqDeleteReaction.h"
#include "pqLoadDataReaction.h"
#include "pqLoadPaletteReaction.h"
// #include "pqSaveDataReaction.h"
// #include "pqSaveExtractsReaction.h"
#include "pqSaveStateReaction.h"
#include "pqServerConnectReaction.h"
#include "pqServerDisconnectReaction.h"
// #include "pqUndoRedoReaction.h"

#include <QToolButton>

//-----------------------------------------------------------------------------
void pqMainControlsToolbar::constructor()
{
  Ui::pqMainControlsToolbar ui;
  ui.setupUi(this);
  new pqLoadDataReaction(ui.actionOpenData);
#if 0 // FIXME ASYNC
  new pqSaveDataReaction(ui.actionSaveData);
#else
  ui.actionSaveData->setEnabled(false);
#endif
  new pqServerConnectReaction(ui.actionServerConnect);
  new pqServerDisconnectReaction(ui.actionServerDisconnect);
#if 0 // FIXME ASYNC
  new pqDeleteReaction(ui.actionDelete, true);
  new pqUndoRedoReaction(ui.actionUndo, true);
  new pqUndoRedoReaction(ui.actionRedo, false);
#else
  ui.actionDelete->setEnabled(false);
  ui.actionUndo->setEnabled(false);
  ui.actionRedo->setEnabled(false);
#endif
  new pqAutoApplyReaction(ui.actionAutoApply);
#if 0 // FIXME ASYNC
  new pqDataQueryReaction(ui.actionQuery);
#else
  ui.actionQuery->setEnabled(false);
#endif
  new pqLoadPaletteReaction(ui.actionLoadPalette);
  new pqSaveStateReaction(ui.actionSaveState);
#if 0 // FIXME ASYNC
  new pqSaveExtractsReaction(ui.actionGenerateExtracts);
#else
  ui.actionGenerateExtracts->setEnabled(false);
#endif
#if VTK_MODULE_ENABLE_ParaView_pqPython
  new pqCatalystExportReaction(ui.actionSaveCatalystState);
#else
  ui.actionSaveCatalystState->setEnabled(false);
#endif

  QToolButton* tb = qobject_cast<QToolButton*>(this->widgetForAction(ui.actionLoadPalette));
  if (tb)
  {
    tb->setPopupMode(QToolButton::InstantPopup);
  }
}
