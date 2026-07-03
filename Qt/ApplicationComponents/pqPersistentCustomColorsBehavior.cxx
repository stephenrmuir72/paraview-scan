// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPersistentCustomColorsBehavior.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

#include <QColorDialog>
#include <QCoreApplication>

namespace
{
const QString CUSTOM_COLORS_KEY = "CustomColors";
}

//-----------------------------------------------------------------------------
pqPersistentCustomColorsBehavior::pqPersistentCustomColorsBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this,
    &pqPersistentCustomColorsBehavior::saveCustomColors);

  pqPersistentCustomColorsBehavior::restoreCustomColors();
}

//-----------------------------------------------------------------------------
pqPersistentCustomColorsBehavior::~pqPersistentCustomColorsBehavior() = default;

//-----------------------------------------------------------------------------
void pqPersistentCustomColorsBehavior::restoreCustomColors()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  const QStringList colors = settings->value(CUSTOM_COLORS_KEY).toStringList();
  for (int i = 0; i < colors.size() && i < QColorDialog::customCount(); ++i)
  {
    const QColor color(colors[i]);
    if (color.isValid())
    {
      QColorDialog::setCustomColor(i, color);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPersistentCustomColorsBehavior::saveCustomColors()
{
  QStringList colors;
  for (int i = 0; i < QColorDialog::customCount(); ++i)
  {
    colors << QColorDialog::customColor(i).name(QColor::HexArgb);
  }
  pqApplicationCore::instance()->settings()->setValue(CUSTOM_COLORS_KEY, colors);
}
