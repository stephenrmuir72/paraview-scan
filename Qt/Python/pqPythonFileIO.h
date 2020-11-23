/*=========================================================================

   Program: ParaView
   Module:    pqPythonFileIO.h

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

#ifndef pqPythonTextSave_h
#define pqPythonTextSave_h

#include "pqPythonModule.h"

#include <QAction>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTextEdit>

#include <array>

class PQPYTHON_EXPORT pqPythonFileIO : public QObject
{
  Q_OBJECT
public:
  enum class IOAction : std::uint8_t
  {
    NewFile = 0,
    OpenFile,
    SaveFile,
    SaveFileAs,
    SaveFileAsMacro,
    END
  };

  pqPythonFileIO(QObject* parent, QTextEdit& text);

  QAction* GetAction(const IOAction action) { return &Actions[action]; }

  bool SaveOnClose();

  bool OpenFile(const QString& filename);

  void SetDefaultSaveDirectory(const QString& dir) { this->DefaultSaveDirectory = dir; }

signals:
  void BufferErased();
  void FileOpened(const QString&);
  void FileSaved(const QString&);
  void FileSavedAsMacro(const QString&);

private slots:
  bool NewFile();
  bool Open();
  bool Save();
  bool SaveAs();
  bool SaveAsMacro();

private:
  bool LoadFile(const QString& filename);

  bool SaveFile(const QString& filename);

  bool NeedSave() const { return this->TextEdit.document()->isModified(); }

  void SetModified(bool modified) { this->TextEdit.document()->setModified(modified); }

  QTextEdit& TextEdit;

  QString DefaultSaveDirectory = QDir::homePath();

  QString Filename = "";

  template <typename E, class T, std::size_t N = static_cast<size_t>(E::END)>
  struct EnumArray : public std::array<T, N>
  {
    T& operator[](E e) { return std::array<T, N>::operator[](static_cast<size_t>(e)); }
  };

  EnumArray<IOAction, QAction> Actions;
};

#endif // pqPythonTextSave_h
