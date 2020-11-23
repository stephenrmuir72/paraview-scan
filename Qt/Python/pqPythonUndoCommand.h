/*=========================================================================

   Program: ParaView
   Module:    pqPythonUndoStack.h

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
#ifndef pqPythonUndoStack_h
#define pqPythonUndoStack_h

#include "pqPythonModule.h"

#include <QTextEdit>
#include <QUndoCommand>

#include <functional>
#include <iostream>
#include <stack>
#include <vector>

class pqPythonSyntaxHighlighter;

/**
 * @struct PythonTextHistory
 * @brief POD data struct that contains the command data to perform undo/redo operations
 */
struct pqPythonTextHistoryEntry
{
  pqPythonTextHistoryEntry() = default;

  pqPythonTextHistoryEntry(const QString& txt, const std::int32_t cursorPos)
    : content(txt)
    , cursorPosition(cursorPos)
  {
  }

  /**
   * @brief The raw text
   */
  QString content = "";

  /**
   * @brief The absolute cursor position within \ref pqPythonUndoCommand::text
   */
  std::int32_t cursorPosition = -1;

  /**
   * @brief Returns true is the entry is empty
   */
  bool IsEmpty() const { return cursorPosition == -1; }
};

/**
 * @class pqPythonUndoCommand
 * @brief The python text editor undo/redo command
 * @detail The \ref pqPythonUndoCommand models the undo/redo framework needed for QUndoStack.
 * The actual storage of the \ref PythonTextHistory is external to the command: this avoids
 * duplicating entries.
 */
class PQPYTHON_EXPORT pqPythonUndoCommand : public QUndoCommand
{
public:
  /**
   * @brief Construct an undo command
   */
  pqPythonUndoCommand(QTextEdit& text, pqPythonSyntaxHighlighter* highlighter,
    pqPythonTextHistoryEntry& lastHistoryEntry,
    const pqPythonTextHistoryEntry&& currentHistoryEntry);

  /**
   * @brief Overriden function that performs the undo
   */
  void undo() override;

  /**
   * @brief Overriden function that performs the redo
   */
  void redo() override;

  /**
   * @brief Returns the current history entry
   */
  const pqPythonTextHistoryEntry& GetCurrentHistoryEntry() const
  {
    return this->CurrentHistoryEntry;
  }

private:
  /**
   * @brief Utility function that mutates the \ref text
   * using the given history value \ref h.
   */
  void Swap_Impl(const pqPythonTextHistoryEntry& h);

  /// @brief The text to do the undo/redo on
  QTextEdit& Text;

  /**
   * @brief The text syntax highlighter
   */
  pqPythonSyntaxHighlighter* Highlighter;

  /**
   * @brief The last history entry in the text
   * @details This is a direct reference to the history entry
   * contained in the text. This is being modified when performing
   * the undo function.
   */
  pqPythonTextHistoryEntry& TextLastHystoryEntry;

  /**
   * @brief The last version of the text
   */
  const pqPythonTextHistoryEntry LastHistoryEntry;

  /**
   * @brief The current version of the text
   */
  const pqPythonTextHistoryEntry CurrentHistoryEntry;
};

#endif // pqPythonUndoStack_h
