/*=========================================================================

   Program: ParaView
   Module:    pqPythonScriptEditor.h

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
#ifndef _pqPythonScriptEditor_h
#define _pqPythonScriptEditor_h

#include "pqPythonModule.h"

#include <QMainWindow>

class QAction;
class QMenu;

class pqPythonManager;
class pqPythonTextArea;

class PQPYTHON_EXPORT pqPythonScriptEditor : public QMainWindow
{
  Q_OBJECT

public:
  explicit pqPythonScriptEditor(QWidget* parent = nullptr);

  void setSaveDialogDefaultDirectory(const QString& dir);
  void setPythonManager(pqPythonManager* manager);

  /*
   * Scroll the editor to the bottom of the scroll area
   */
  void scrollToBottom();

  void open(const QString& filename);

public Q_SLOTS:
  void setText(const QString& text);

protected:
  void closeEvent(QCloseEvent* event) override;

private:
  void createActions();
  void createMenus();
  void createStatusBar();

  pqPythonTextArea* TextArea;

  QMenu* fileMenu;
  QMenu* editMenu;
  QMenu* helpMenu;

  QAction* exitAct;
  QAction* cutAct;
  QAction* copyAct;
  QAction* pasteAct;

  pqPythonManager* pythonManager;
};

#endif
