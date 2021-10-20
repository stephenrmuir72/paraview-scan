/*=========================================================================

   Program: ParaView
   Module:  pqColorPaletteSelectorWidget.cxx

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
#include "pqColorPaletteSelectorWidget.h"

#include "vtkLogger.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QComboBox>
#include <QVBoxLayout>

#include <cassert>

#define PQ_LOAD_PALETTE_DETAIL_MACRO(name)                                                         \
  if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", name))                            \
  {                                                                                                \
    cbbox->addItem(prototype->GetXMLLabel(), prototype->GetXMLName());                             \
    mainPalettes.insert(std::string(name));                                                        \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    vtkLog(WARNING, "Missing palette: " << name);                                                  \
  }

//-----------------------------------------------------------------------------
pqColorPaletteSelectorWidget::pqColorPaletteSelectorWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  // this->setShowLabel(false);

  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setSpacing(0);
  vbox->setMargin(0);

  vtkSMSessionProxyManager* pxm = smproxy->GetSessionProxyManager();
  vtkSMProxyDefinitionManager* pdmgr = pxm->GetProxyDefinitionManager();

  QComboBox* cbbox = new QComboBox(this);
  cbbox->setObjectName("ComboBox");

  // We want a certain ordering for the palettes in the combo box (see issue #20707), so
  // we save which ones were already added in this std::set
  std::set<std::string> mainPalettes;

  // We do not want to display Default (Blue Gray) and Print (White) twice,
  // so we add them to the list of inserted palettes without actually adding them to the combo box.
  mainPalettes.insert("DefaultBackground");
  mainPalettes.insert("PrintBackground");

  PQ_LOAD_PALETTE_DETAIL_MACRO("BlueGrayBackground");
  PQ_LOAD_PALETTE_DETAIL_MACRO("WarmGrayBackground");
  PQ_LOAD_PALETTE_DETAIL_MACRO("NeutralGrayBackground");
  PQ_LOAD_PALETTE_DETAIL_MACRO("LightGrayBackground");
  PQ_LOAD_PALETTE_DETAIL_MACRO("WhiteBackground");
  PQ_LOAD_PALETTE_DETAIL_MACRO("BlackBackground");
  PQ_LOAD_PALETTE_DETAIL_MACRO("GradientBackground");

  assert(pdmgr);
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pdmgr->NewSingleGroupIterator("palettes"));

  // If there were any other available palettes, we append them in alphabetical order.
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", iter->GetProxyName());
    if (!mainPalettes.count(std::string(prototype->GetXMLName())))
    {
      cbbox->addItem(prototype->GetXMLLabel(), prototype->GetXMLName());
    }
  }

  if (cbbox->count() > 0)
  {
    cbbox->insertItem(0, "Select palette to load ...", -1);
  }
  else
  {
    cbbox->insertItem(0, "No palettes available.");
    cbbox->setEnabled(false);
  }
  cbbox->setCurrentIndex(0);
  vbox->addWidget(cbbox);

  this->ComboBox = cbbox;

  // If a SVP is provided, then we will simply change is value to the selected palette,
  // otherwise we'll act as a "loader".
  if (vtkSMStringVectorProperty::SafeDownCast(smproperty))
  {
    this->addPropertyLink(this, "paletteName", SIGNAL(paletteNameChanged()), smproperty);
    this->connect(cbbox, SIGNAL(currentIndexChanged(int)), SIGNAL(paletteNameChanged()));
    cbbox->setItemText(0, "No change");
  }
  else
  {
    this->connect(cbbox, SIGNAL(currentIndexChanged(int)), SLOT(loadPalette(int)));
  }
}

#undef PQ_LOAD_PALETTE_DETAIL_MACRO

//-----------------------------------------------------------------------------
pqColorPaletteSelectorWidget::~pqColorPaletteSelectorWidget() = default;

//-----------------------------------------------------------------------------
QString pqColorPaletteSelectorWidget::paletteName() const
{
  int index = this->ComboBox->currentIndex();
  if (index <= 0)
  {
    return QString(); // none selected.
  }

  return this->ComboBox->itemData(index).toString();
}

//-----------------------------------------------------------------------------
void pqColorPaletteSelectorWidget::setPaletteName(const QString& pname)
{
  int index = this->ComboBox->findData(QVariant(pname));
  this->ComboBox->setCurrentIndex(index <= 0 ? 0 : index);
}

//-----------------------------------------------------------------------------
void pqColorPaletteSelectorWidget::loadPalette(int index)
{
  vtkSMProxy* smproxy = this->proxy();
  assert(this->ComboBox);
  assert(smproxy);

  if (index <= 0)
  {
    return;
  }
  QString name = this->ComboBox->itemData(index).toString();

  vtkSMSessionProxyManager* pxm = smproxy->GetSessionProxyManager();
  vtkSMProxy* paletteProxy = pxm->GetPrototypeProxy("palettes", name.toUtf8().data());
  assert(paletteProxy);

  smproxy->Copy(paletteProxy);

  // return the combobox back to the "select .." text.
  this->ComboBox->setCurrentIndex(0);

  Q_EMIT this->changeAvailable();
}
