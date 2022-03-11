/*=========================================================================

   Program: ParaView
   Module:  pqColorArrayPropertyWidget.cxx

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
#include "pqColorArrayPropertyWidget.h"
#include "ui_pqColorArrayPropertyWidget.h"

#include "pqCoreUtilities.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVLogger.h"
#include "vtkSMApplyController.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkScalarsToColors.h"

#include <QSignalBlocker>
#include <cassert>

class pqConnection : public pqPropertyLinksConnection
{
public:
  using pqPropertyLinksConnection::pqPropertyLinksConnection;

  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    assert((value.canConvert<QPair<int, QString>>()));
    auto pair = qvariant_cast<QPair<int, QString>>(value);

    vtkSMPropertyHelper helper(this->propertySM());
    helper.SetUseUnchecked(use_unchecked);
    helper.Set(0, std::to_string(pair.first).c_str());
    helper.Set(1, qUtf8Printable(pair.second));
  }

  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    vtkSMPropertyHelper helper(this->propertySM());
    helper.SetUseUnchecked(use_unchecked);
    return QVariant::fromValue(qMakePair(helper.GetAsInt(0), QString(helper.GetAsString(1))));
  }

private:
  Q_DISABLE_COPY(pqConnection);
};

class pqColorArrayPropertyWidget::pqInternals
{
  QList<std::tuple<int, int, QString>> DomainValues;

public:
  Ui::ColorArrayPropertyWidget Ui;
  vtkSmartPointer<vtkSMArrayListDomain> Domain;
  vtkSmartPointer<vtkSMProxy> LookupTable;
  QPair<int, QString> ColorArray;
  int VectorMode{ vtkScalarsToColors::MAGNITUDE };
  int Component{ 0 };

  QPair<int, QString> currentArray() const
  {
    auto data = this->Ui.name->currentData();
    if (!data.isValid() || data.toInt() == -1)
    {
      return qMakePair(0, QString());
    }
    auto& domainValue = this->DomainValues[data.toInt()];
    return qMakePair(std::get<1>(domainValue), std::get<2>(domainValue));
  }

  std::tuple<int, int> currentComponent() const
  {
    auto data = this->Ui.component->currentData();
    if (!data.isValid() || data.toInt() == -1)
    {
      return std::make_tuple(vtkScalarsToColors::MAGNITUDE, this->Component);
    }

    return std::make_tuple(vtkScalarsToColors::COMPONENT, data.toInt());
  }

  pqSMProxy currentLookupTable(vtkSMSessionProxyManager* pxm) const
  {
    if (!this->Ui.mapScalars->isChecked() || this->ColorArray.second.isEmpty())
    {
      return nullptr;
    }

    vtkNew<vtkSMTransferFunctionManager> mgr;
    return mgr->GetColorTransferFunction(qUtf8Printable(this->ColorArray.second), pxm);
  }

  void updateUI();
  void updateComponentsUI();
  void updateDomain();
};

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::pqInternals::updateDomain()
{
  this->DomainValues.clear();
  if (!this->Domain)
  {
    return;
  }

  const auto numValues = this->Domain->GetNumberOfStrings();
  for (unsigned int cc = 0; cc < numValues; ++cc)
  {
    this->DomainValues.push_back(std::make_tuple(this->Domain->GetDomainAssociation(cc), // for icon
      this->Domain->GetFieldAssociation(cc), // for property value
      QString(this->Domain->GetString(cc))));
  }
}

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::pqInternals::updateUI()
{
  auto& ui = this->Ui;
  QSignalBlocker blocker0(ui.name);
  ui.name->clear();

  // populate name menu.
  int currentNameIndex = -1;
  ui.name->addItem(QIcon(":/pqWidgets/Icons/pqSolidColor.svg"), "Solid Color", -1);
  int domainIndex = 0;
  for (const auto& tuple : this->DomainValues)
  {
    ui.name->addItem(
      pqCoreUtilities::fieldAssociationIcon(std::get<0>(tuple)), std::get<2>(tuple), domainIndex++);
    if (this->ColorArray == qMakePair(std::get<1>(tuple), std::get<2>(tuple)))
    {
      currentNameIndex = (ui.name->count() - 1);
    }
  }

  if (currentNameIndex == -1)
  {
    if (this->ColorArray.second.isEmpty())
    {
      // for solid color
      currentNameIndex = 0;
    }
    else if (!this->ColorArray.second.isEmpty())
    {
      // unknown array was chosen.
      ui.name->insertItem(0, pqCoreUtilities::fieldAssociationIcon(this->ColorArray.first),
        QString("%1 (?)").arg(this->ColorArray.second), -1);
      currentNameIndex = 0;
    }
  }

  ui.name->setCurrentIndex(currentNameIndex);
  this->updateComponentsUI();
}

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::pqInternals::updateComponentsUI()
{
  auto& ui = this->Ui;
  QSignalBlocker blocker1(ui.component);
  ui.component->clear();
  ui.component->hide();

  // populate component menu.
  if (this->ColorArray.second.isEmpty() || this->Domain == nullptr || !ui.mapScalars->isChecked())
  {
    return;
  }

  // get array information from the domain.
  auto* arrayInfo = this->Domain->GetArrayInformation(
    this->ColorArray.first, qUtf8Printable(this->ColorArray.second));
  if (!arrayInfo || arrayInfo->GetNumberOfComponents() <= 1)
  {
    return;
  }

  ui.component->show();
  const auto numComponents = arrayInfo->GetNumberOfComponents();
  ui.component->addItem("Magnitude", -1);
  for (int cc = 0; cc < numComponents; ++cc)
  {
    ui.component->addItem(arrayInfo->GetComponentName(cc), cc);
  }

  if (this->VectorMode == vtkScalarsToColors::MAGNITUDE)
  {
    ui.component->setCurrentIndex(0);
  }
  else
  {
    if (this->Component >= 0 && this->Component < numComponents)
    {
      ui.component->setCurrentIndex(this->Component + 1);
    }
    else
    {
      // if chosen component is unknown for the current domain, we still need to
      // show it in the UI with "(?)" suffix.
      ui.component->addItem(QString("%1 (?)").arg(this->Component), this->Component);
      ui.component->setCurrentIndex(ui.component->count() - 1);
    }
  }
}

//=============================================================================
pqColorArrayPropertyWidget::pqColorArrayPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqColorArrayPropertyWidget::pqInternals())
{
  vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "using pqColorArrayPropertyWidget.");
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);

  QObject::connect(internals.Ui.name, QOverload<int>::of(&QComboBox::currentIndexChanged),
    [this, smproxy, &internals](int index) {
      internals.ColorArray = internals.currentArray();

      // when color array is changed, we may need to change to the LUT too.
      this->setLookupTable(internals.currentLookupTable(smproxy->GetSessionProxyManager()));

      internals.updateComponentsUI();
      Q_EMIT this->colorArrayChanged();
    });

  QObject::connect(internals.Ui.component, QOverload<int>::of(&QComboBox::currentIndexChanged),
    [this, &internals](int index) {
      std::tie(internals.VectorMode, internals.Component) = internals.currentComponent();
      Q_EMIT this->colorArrayComponentChanged();
    });

  QObject::connect(
    internals.Ui.mapScalars, &QAbstractButton::toggled, [this, &internals, smproxy](bool) {
      // we may need to setup lookuptable
      this->setLookupTable(internals.currentLookupTable(smproxy->GetSessionProxyManager()));
      internals.updateComponentsUI();
    });

  if (auto* colorArrayName = smgroup->GetProperty("ColorArrayName"))
  {
    auto* domain = colorArrayName->FindDomain<vtkSMArrayListDomain>();
    if (domain)
    {
      internals.Domain = domain;
      pqCoreUtilities::connect(
        domain, vtkCommand::DomainModifiedEvent, this, SLOT(domainUpdated()));
      internals.updateDomain();
    }

    this->links().addPropertyLink<pqConnection>(
      this, "colorArray", SIGNAL(colorArrayChanged()), smproxy, colorArrayName);
  }
  else
  {
    internals.Ui.name->hide();
  }

  if (auto* mapScalars = smgroup->GetProperty("MapScalars"))
  {
    this->addPropertyLink(internals.Ui.mapScalars, "checked", SIGNAL(toggled(bool)), mapScalars);
  }
  else
  {
    internals.Ui.mapScalars->hide();
  }

  if (auto* interpolateScalars = smgroup->GetProperty("InterpolateScalarsBeforeMapping"))
  {
    this->addPropertyLink(
      internals.Ui.interpolateScalars, "checked", SIGNAL(toggled(bool)), interpolateScalars);
  }
  else
  {
    internals.Ui.interpolateScalars->hide();
  }

  if (auto* lookupTable = smgroup->GetProperty("LookupTable"))
  {
    this->addPropertyLink(this, "lookupTable", SIGNAL(lookupTableChanged()), lookupTable);
  }
}

//-----------------------------------------------------------------------------
pqColorArrayPropertyWidget::~pqColorArrayPropertyWidget() = default;

//-----------------------------------------------------------------------------
QPair<int, QString> pqColorArrayPropertyWidget::colorArray() const
{
  const auto& internals = (*this->Internals);
  return internals.ColorArray;
}

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::setColorArray(const QPair<int, QString>& value)
{
  auto& internals = (*this->Internals);
  internals.ColorArray = value;
  internals.updateUI();
}

//-----------------------------------------------------------------------------
pqSMProxy pqColorArrayPropertyWidget::lookupTable() const
{
  const auto& internals = (*this->Internals);
  return internals.LookupTable;
}

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::setLookupTable(const pqSMProxy& proxy)
{
  auto& internals = (*this->Internals);
  if (internals.LookupTable == proxy)
  {
    return;
  }

  auto& plinks = this->links();
  if (internals.LookupTable)
  {
    plinks.removePropertyLink(this, "colorArrayComponent", SIGNAL(colorArrayComponentChanged()),
      internals.LookupTable, internals.LookupTable->GetProperty("VectorComponent"));
    plinks.removePropertyLink(this, "vectorMode", SIGNAL(colorArrayComponentChanged()),
      internals.LookupTable, internals.LookupTable->GetProperty("VectorMode"));
  }

  internals.LookupTable = proxy;

  if (internals.LookupTable)
  {
    plinks.addPropertyLink(this, "colorArrayComponent", SIGNAL(colorArrayComponentChanged()),
      internals.LookupTable, internals.LookupTable->GetProperty("VectorComponent"));
    plinks.addPropertyLink(this, "vectorMode", SIGNAL((colorArrayComponentChanged)),
      internals.LookupTable, internals.LookupTable->GetProperty("VectorMode"));
  }

  Q_EMIT this->lookupTableChanged();
}

//-----------------------------------------------------------------------------
int pqColorArrayPropertyWidget::colorArrayComponent() const
{
  const auto& internals = (*this->Internals);
  return internals.Component;
}

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::setColorArrayComponent(int component)
{
  auto& internals = (*this->Internals);
  internals.Component = component;
  internals.updateUI();
}

//-----------------------------------------------------------------------------
int pqColorArrayPropertyWidget::vectorMode() const
{
  const auto& internals = (*this->Internals);
  return internals.VectorMode;
}

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::setVectorMode(int mode)
{
  auto& internals = (*this->Internals);
  internals.VectorMode = mode;
  internals.updateUI();
}

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::domainUpdated()
{
  auto& internals = (*this->Internals);
  internals.updateDomain();
  internals.updateUI();
}

//-----------------------------------------------------------------------------
void pqColorArrayPropertyWidget::apply()
{
  this->Superclass::apply();
  // TODO: if transfer function should be reset then
  // reset the range.

  vtkNew<vtkSMApplyController> controller;
  controller->ApplyScalarColoring(this->proxy());
}
