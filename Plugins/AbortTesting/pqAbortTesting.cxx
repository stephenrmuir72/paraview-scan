// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAbortTesting.h"

#include "pqApplicationCore.h"
#include "pqProgressManager.h"
#include "pqSettings.h"

#include <iostream>
//-----------------------------------------------------------------------------
pqAbortTesting::pqAbortTesting(QObject* parent)
  : QObject(parent)
{
}

//-----------------------------------------------------------------------------
pqAbortTesting::~pqAbortTesting() = default;

//-----------------------------------------------------------------------------
void pqAbortTesting::onStartup()
{
  pqProgressManager* pmManager = pqApplicationCore::instance()->getProgressManager();
  QObject::connect(pmManager, &pqProgressManager::progress, this, &pqAbortTesting::onProgress);
}

//-----------------------------------------------------------------------------
void pqAbortTesting::onProgress(const QString&, int progress)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  //  bool autoAbort = vtkAbortTestingSettings::GetInstance()->GetAutoAbort();
  bool autoAbort = settings->value("AbortTestingSettings.AutoAbort", true).toBool();
  std::cout << "autoAbort: " << autoAbort << std::endl;
  //  int autoAbortValue = vtkAbortTestingSettings::GetInstance()->GetAutoAbortValue();
  int autoAbortValue = settings->value("AbortTestingSettings.AutoAbortValue", 50).toInt();
  std::cout << "autoAbortValue: " << autoAbortValue << std::endl;
  if (autoAbort && progress > autoAbortValue)
  {
    pqProgressManager* pmManager = pqApplicationCore::instance()->getProgressManager();
    pmManager->triggerAbort();
  }
}
