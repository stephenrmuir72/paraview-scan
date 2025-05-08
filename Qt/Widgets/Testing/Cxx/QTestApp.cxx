// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "QTestApp.h"

#include <cstdio>

#include "vtkStringFormatter.h"

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QWidget>

int QTestApp::Error = 0;

QTestApp::QTestApp(int _argc, char** _argv)
{
  qInstallMessageHandler(QTestApp::messageHandler);

  // CMake generated driver removes argv[0],
  // so let's put a dummy back in
  this->Argv.append("qTestApp");
  for (int i = 0; i < _argc; i++)
  {
    this->Argv.append(_argv[i]);
  }
  for (int j = 0; j < this->Argv.size(); j++)
  {
    this->Argvp.append(this->Argv[j].data());
  }
  this->Argc = this->Argvp.size();
  App = new QApplication(this->Argc, this->Argvp.data());
}

QTestApp::~QTestApp()
{
  delete App;
  qInstallMessageHandler(nullptr);
}

int QTestApp::exec()
{
  if (QCoreApplication::arguments().contains("--exit"))
  {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
  }
  return Error + QApplication::exec();
}

void QTestApp::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  QByteArray localMsg = msg.toUtf8();
  const char* message = !localMsg.isEmpty() ? localMsg.data() : "";
  const char* file = context.file ? context.file : "";
  const int line = context.line;
  const char* function = context.function ? context.function : "";
  switch (type)
  {
    case QtDebugMsg:
      vtk::print(stderr, "Debug: {:s} ({:s}:{:d}, {:s})\n", message, file, line, function);
      break;
    case QtInfoMsg:
      vtk::print(stderr, "Info: {:s} ({:s}:{:d}, {:s})\n", message, file, line, function);
      break;
    case QtWarningMsg:
      vtk::print(stderr, "Warning: {:s} ({:s}:{:d}, {:s})\n", message, file, line, function);
      break;
    case QtCriticalMsg:
      vtk::print(stderr, "Critical: {:s} ({:s}:{:d}, {:s})\n", message, file, line, function);
      Error++;
      break;
    case QtFatalMsg:
      vtk::print(stderr, "Fatal: {:s} ({:s}:{:d}, {:s})\n", message, file, line, function);
      abort();
  }
}

void QTestApp::delay(int ms)
{
  if (ms > 0)
  {
    QTimer::singleShot(ms, QApplication::instance(), SLOT(quit()));
    QApplication::exec();
  }
}

bool QTestApp::simulateEvent(QWidget* w, QEvent* e)
{
  bool status = QApplication::sendEvent(w, e);
  QApplication::processEvents();
  return status;
}

QString QTestApp::keyToAscii(Qt::Key key, Qt::KeyboardModifiers mod)
{
  QString text;
  char off = 'a';
  if (mod & Qt::ShiftModifier)
    off = 'A';
  if (key >= Qt::Key_A && key <= Qt::Key_Z)
  {
    text.append(QLatin1Char(key - Qt::Key_A + off));
  }
  return text;
}

void QTestApp::keyUp(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
    return;
  delay(ms);
  QKeyEvent e(QEvent::KeyRelease, key, mod, keyToAscii(key, mod));
  if (!simulateEvent(w, &e))
  {
    qWarning("keyUp not handled\n");
  }
}

void QTestApp::keyDown(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
    return;
  delay(ms);
  QKeyEvent e(QEvent::KeyPress, key, mod, keyToAscii(key, mod));
  if (!simulateEvent(w, &e))
  {
    qWarning("keyDown not handled\n");
  }
}

void QTestApp::keyClick(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  keyDown(w, key, mod, 0);
  keyUp(w, key, mod, 0);
}

void QTestApp::keyClicks(QWidget* w, const QString& text, Qt::KeyboardModifiers mod, int ms)
{
  for (int i = 0; i < text.length(); ++i)
  {
    QChar letter = text.at(i);
    Qt::Key key =
      static_cast<Qt::Key>(Qt::Key_A + letter.toLower().unicode() - QChar('a').unicode());
    Qt::KeyboardModifiers upper = letter.isUpper() ? Qt::ShiftModifier : Qt::NoModifier;
    keyClick(w, key, mod | upper, ms);
  }
}

void QTestApp::mouseDown(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonPress, pos, w->mapToGlobal(pos), btn, btn, mod);
  if (!simulateEvent(w, &e))
  {
    qWarning("mouseDown not handled\n");
  }
}

void QTestApp::mouseUp(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonRelease, pos, w->mapToGlobal(pos), btn, btn, mod);
  if (!simulateEvent(w, &e))
  {
    qWarning("mouseUp not handled\n");
  }
}

void QTestApp::mouseMove(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseMove, pos, w->mapToGlobal(pos), btn, btn, mod);
  if (!simulateEvent(w, &e))
  {
    qWarning("mouseMove not handled\n");
  }
}

void QTestApp::mouseClick(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  mouseDown(w, pos, btn, mod, 0);
  mouseUp(w, pos, btn, mod, 0);
}

void QTestApp::mouseDClick(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonDblClick, pos, w->mapToGlobal(pos), btn, btn, mod);
  if (!simulateEvent(w, &e))
  {
    qWarning("mouseMove not handled\n");
  }
}
