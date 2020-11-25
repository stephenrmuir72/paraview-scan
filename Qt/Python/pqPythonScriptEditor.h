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

#include <pqCoreUtilities.h>

#include <QMainWindow>

class QAction;
class QMenu;

class pqPythonManager;
class pqPythonTextArea;

/**
 * @class pqPythonScriptEditor
 * @brief Paraview Python Editor widget
 */
class PQPYTHON_EXPORT pqPythonScriptEditor : public QMainWindow
{
  Q_OBJECT

public:
  explicit pqPythonScriptEditor(QWidget* parent = nullptr);

  /**
   * @brief Sets the default save directory for the current buffer
   */
  void setSaveDialogDefaultDirectory(const QString& dir);

  /**
   * @brief Sets the \ref pqPythonManager used for the macros
   */
  void setPythonManager(pqPythonManager* manager);

  /**
   * @brief Scroll the editor to the bottom of the scroll area
   */
  void scrollToBottom();

  /**
   * @brief Open a file inside the editor
   */
  void open(const QString& filename);

  /**
   * @brief Utility function that provides a single instance
   * of the editor
   */
  static pqPythonScriptEditor* GetUniqueInstance()
  {
    static pqPythonScriptEditor* instance = new pqPythonScriptEditor(pqCoreUtilities::mainWidget());
    return instance;
  }

public Q_SLOTS:
  /**
   * @brief Q_SLOT that sets the current buffer to \ref text
   * @details Override the current buffer (even if it is not saved).
   * This method is only used by the \ref pqTraceAction widget.
   */
  void setText(const QString& text);

protected:
  /**
   * @brief Override the QMainWindow closeEvent
   * @details We ask the user wants to save the current
   * file if it's not already saved
   */
  void closeEvent(QCloseEvent* event) override;

private:
  /**
   * @brief Setup the widget actions
   */
  void createActions();

  /**
   * @brief Setup the widget menus
   */
  void createMenus();

  /**
   * @brief Setup the status bar
   */
  void createStatusBar();

  /**
   * The central text widget
   */
  pqPythonTextArea* TextArea;

  QMenu* fileMenu;
  QMenu* editMenu;
  QMenu* helpMenu;

  QAction* exitAct;
  QAction* cutAct;
  QAction* copyAct;
  QAction* pasteAct;

  /**
   * The python manager only used for the
   * paraview macro system
   */
  pqPythonManager* pythonManager;
};

#endif
