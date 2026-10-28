// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqThreadedUpdateBehavior_h
#define pqThreadedUpdateBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QPointer>

#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"

class pqServer;

/**
 * @ingroup Behaviors
 * pqThreadedUpdateBehavior let the data update code running in a separate thread
 * and keep the main thread alive during processing, for a more responsive UI.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqThreadedUpdateBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqThreadedUpdateBehavior(QObject* parent = nullptr);

Q_SIGNALS:

  /**
   * Emited when a TODO event is received
   */
  void endLoop();

protected Q_SLOTS:

  /**
   * Connect the TODO event to the temporary event loop creation and destruction
   */
  void onServerCreation(pqServer*);

  /**
   * Run a QEventLoop until endLoop is received
   */
  void loop();

private:
  Q_DISABLE_COPY(pqThreadedUpdateBehavior)

  vtkNew<vtkEventQtSlotConnect> VTKConnector;
};

#endif
