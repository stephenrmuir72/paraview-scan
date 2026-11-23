// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqNewPresetDialog.h"
#include "ui_pqNewPresetDialog.h"

#include "vtkSMSettings.h"
#include <QPushButton>
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

  this->connect(this->Internals->Ui.PresetName, &QLineEdit::textChanged,
    [this](const QString& text)
    {
      // Warning
      if (std::find(this->usedNames.begin(), this->usedNames.end(), text.toStdString()) ==
        this->usedNames.end())
      {
        this->Internals->Ui.PresetWarning->hide();
      }
      else
      {
        this->Internals->Ui.PresetWarning->show();
      }

      // Don't allow user to use an empty preset name
      this->Internals->Ui.ButtonBox->button(QDialogButtonBox::StandardButton::Ok)
        ->setDisabled(text.isEmpty());
    });

  this->Internals->Ui.PresetWarning->hide();
  this->Internals->Ui.ButtonBox->button(QDialogButtonBox::StandardButton::Ok)->setDisabled(true);
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

//-----------------------------------------------------------------------------
void pqNewPresetDialog::setUsedNames(const std::vector<std::string>& usedNames)
{
  this->usedNames = usedNames;
}
