/*=========================================================================

   Program: ParaView
   Module:  pqModalShortcut.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqModalShortcut.h"

#include "pqProxy.h"
#include <QAction>
#include <QShortcut>
#include <QWidget>

pqModalShortcut::pqModalShortcut(const QKeySequence& key, QAction* action, QWidget* parent)
  : Superclass(parent)
  , m_key(key)
  , m_action(action)
{
  m_shortcut = new QShortcut(key, parent);
  // if parent, look for slots to auto-connect
  // if action, connect shortcut.
  QObject::connect(m_shortcut, &QShortcut::activated, this, &pqModalShortcut::activated);
  if (m_action)
  {
    QObject::connect(m_shortcut, &QShortcut::activated, m_action, &QAction::trigger);
  }
}

pqModalShortcut::~pqModalShortcut()
{
  emit unregister();
  delete m_shortcut;
}

void pqModalShortcut::setContextWidget(QWidget* contextWidget, Qt::ShortcutContext contextArea)
{
  bool enabled = this->isEnabled();
  if (m_shortcut)
  {
    if (m_shortcut->parentWidget() == contextWidget)
    {
      if (m_shortcut->context() != contextArea)
      {
        m_shortcut->setContext(contextArea);
      }
      return;
    }
  }

  // To change parents, it's best to start over.
  delete m_shortcut;
  if (!contextWidget && contextArea != Qt::ApplicationShortcut)
  {
    // We need to keep a shortcut around, but don't pay attention
    // to it since the context widget is null.
    m_shortcut = nullptr;
  }
  else
  {
    m_shortcut = new QShortcut(m_key, contextWidget);
    m_shortcut->setEnabled(enabled);
    m_shortcut->setContext(contextArea);
    QObject::connect(m_shortcut, &QShortcut::activated, this, &pqModalShortcut::activated);
    if (m_action)
    {
      QObject::connect(m_shortcut, &QShortcut::activated, m_action, &QAction::trigger);
    }
  }
}

bool pqModalShortcut::isEnabled() const
{
  return m_shortcut ? m_shortcut->isEnabled() : false;
}

void pqModalShortcut::setEnabled(bool enable)
{
  if (!m_shortcut)
  {
    // View was destroyed.
    return;
  }

  if (enable)
  {
    if (!m_shortcut->isEnabled())
    {
      emit enabled();
      m_shortcut->setEnabled(true);
    }
  }
  else
  {
    if (m_shortcut->isEnabled())
    {
      m_shortcut->setEnabled(false);
      emit disabled();
    }
  }
}

QKeySequence pqModalShortcut::keySequence() const
{
  return m_key;
}
