// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqNewPresetDialog_h
#define pqNewPresetDialog_h

#include "pqComponentsModule.h"
#include "pqDialog.h"

/**
 * @brief pqNewPresetDialog is a dialog window that is used to create a new
 * settings preset.
 */
class PQCOMPONENTS_EXPORT pqNewPresetDialog : public pqDialog
{
  Q_OBJECT
  typedef pqDialog Superclass;

public:
  pqNewPresetDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~pqNewPresetDialog() override;

  /**
   * Return the name of the preset
   */
  const QString& name() { return this->Name; }

public Q_SLOTS:
  /**
   * Store the name of the preset after accept.
   * This slot is connected in pqPropertiesPanel to add a new preset
   */
  void accept() override;

protected:
  QString Name;

private:
  Q_DISABLE_COPY(pqNewPresetDialog)
  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
