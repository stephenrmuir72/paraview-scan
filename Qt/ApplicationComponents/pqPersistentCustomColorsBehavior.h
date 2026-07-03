// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPersistentCustomColorsBehavior_h
#define pqPersistentCustomColorsBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

/**
 * @ingroup Behaviors
 * @brief pqPersistentCustomColorsBehavior saves and restores the custom colors
 * of Qt color dialogs on shutdown and restart.
 *
 * Custom colors added in any color dialog (for example when picking a solid
 * color for a representation) are shared by all `QColorDialog` instances of the
 * application. Qt 5 used to save them in organization-wide settings, but Qt 6
 * only keeps them for the duration of the application. This behavior saves
 * them in the application settings when the application quits and restores
 * them when it is instantiated. Simply instantiate this behavior if you want
 * custom colors to be persistent.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPersistentCustomColorsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPersistentCustomColorsBehavior(QObject* parent = nullptr);
  ~pqPersistentCustomColorsBehavior() override;

  /**
   * Restore the color dialogs custom colors from the application settings.
   */
  static void restoreCustomColors();

  /**
   * Save the color dialogs custom colors in the application settings.
   */
  static void saveCustomColors();

private:
  Q_DISABLE_COPY(pqPersistentCustomColorsBehavior)
};

#endif
