// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqComboBoxStyle.h"

#include <QApplication>
#include <QStyleFactory>

//-----------------------------------------------------------------------------
pqComboBoxStyle::pqComboBoxStyle(bool showPopup)
  : QProxyStyle(QStyleFactory::create(QApplication::style()->name()))
#if QT_VERSION < QT_VERSION_CHECK(6, 1, 0)
  : QProxyStyle(QStyleFactory::create("fusion"))
#else
  , ShowPopup(showPopup)
#endif
{
  setObjectName("pqComboBoxStyle");
}

//-----------------------------------------------------------------------------
pqComboBoxStyle::~pqComboBoxStyle() = default;

//-----------------------------------------------------------------------------
int pqComboBoxStyle::styleHint(
  StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* ret) const
{
  if (hint == QStyle::SH_ComboBox_Popup)
  {
    return this->ShowPopup ? 1 : 0;
  }
  else
  {
    return QProxyStyle::styleHint(hint, option, widget, ret);
  }
}
