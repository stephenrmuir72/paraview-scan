// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqNewPresetDialog.h"
#include "ui_pqNewPresetDialog.h"

#include "vtkSMSettings.h"
#include <algorithm>

//-----------------------------------------------------------------------------
class pqNewPresetDialog::pqInternals
{
public:
  Ui::pqNewPresetDialog Ui;

  pqInternals(pqNewPresetDialog* self) { this->Ui.setupUi(self); }

  ~pqInternals() = default;
};

//-----------------------------------------------------------------------------
pqNewPresetDialog::pqNewPresetDialog(QWidget* parentObject, Qt::WindowFlags flags)
  : Superclass(parentObject, flags)
  , Internals(new pqNewPresetDialog::pqInternals(this))
{
  QObject::connect(
    this->Internals->Ui.ButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  QObject::connect(
    this->Internals->Ui.ButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

//-----------------------------------------------------------------------------
void pqNewPresetDialog::accept()
{
  this->Name = this->Internals->Ui.PresetName->text().trimmed();

  if (this->Name.isEmpty())
  {
    this->done(Rejected);
    return;
  }

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  auto currentNames = settings->GetPresets();
  if (std::find(currentNames.begin(), currentNames.end(), this->Name.toStdString()) !=
    currentNames.end())
  {
    this->done(Rejected);
    return;
  }

  this->done(Accepted);
}

//-----------------------------------------------------------------------------
pqNewPresetDialog::~pqNewPresetDialog()
{
  delete this->Internals;
}
