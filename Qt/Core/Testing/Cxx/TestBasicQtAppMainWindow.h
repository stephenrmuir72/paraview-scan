/*=========================================================================

  Program: ParaView
  Module:  TestBasicQtAppMainWindow.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <QMainWindow>

#include <memory> // for std::unique_ptr
class TestBasicQtAppMainWindow : public QMainWindow
{
  Q_OBJECT;

public:
  TestBasicQtAppMainWindow();
  ~TestBasicQtAppMainWindow();

  void connect(const QString& url);

private:
  Q_DISABLE_COPY(TestBasicQtAppMainWindow);

  class QInternals;
  std::unique_ptr<QInternals> Internals;
};
