/*=========================================================================

  Program: ParaView
  Module:  TestBasicQtAppMainWindow.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestBasicQtAppMainWindow.h"
#include "ui_TestBasicQtAppMainWindow.h"

#include "pqProgressWidget.h"
#include "pqQVTKWidget.h"
#include "vtkClientSession.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

class TestBasicQtAppMainWindow::QInternals
{
public:
  Ui::TestBasicQtAppMainWindow Ui;
  void setupSession(pqQVTKWidget* renderWidget, pqProgressWidget* progressWidget);
};

void TestBasicQtAppMainWindow::QInternals::setupSession(
  pqQVTKWidget* renderWidget, pqProgressWidget* progressWidget)
{
  auto* pvapp = vtkPVApplication::GetInstance();
  auto sessionId = pvapp->CreateBuiltinSession();
  auto* session = pvapp->GetSession(sessionId);
  auto* pxm = session->GetProxyManager();

  auto sphere =
    vtk::TakeSmartPointer(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "SphereSource")));
  sphere->SetLogName("SphereSource");
  vtkSMPropertyHelper(sphere, "PhiResolution").Set(800);
  vtkSMPropertyHelper(sphere, "ThetaResolution").Set(800);
  sphere->UpdateVTKObjects();
  pxm->RegisterProxy("sources", sphere);

  auto shrink =
    vtk::TakeSmartPointer(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ShrinkFilter")));
  shrink->SetLogName("ShrinkFilter");
  vtkSMPropertyHelper(shrink, "Input").Set(sphere, 0);
  shrink->UpdateVTKObjects();
  pxm->RegisterProxy("sources", shrink);

  auto repr = vtk::TakeSmartPointer(pxm->NewProxy("representations", "GeometryRepresentation"));
  repr->SetLogName("Representation (ShrinkFilter)");
  vtkSMPropertyHelper(repr, "Input").Set(shrink, 0);
  repr->UpdateVTKObjects();
  pxm->RegisterProxy("representations", repr);

  auto viewProxy =
    vtk::TakeSmartPointer(vtkSMViewProxy::SafeDownCast(pxm->NewProxy("views", "RenderView")));
  vtkSMPropertyHelper(viewProxy, "Representations").Add(repr);
  viewProxy->UpdateVTKObjects();

  renderWidget->setRenderWindow(viewProxy->GetRenderWindow());

  viewProxy->InvokeCommand("Update");
  pxm->RegisterProxy("views", viewProxy);

  auto observable = session->GetProgressObservable().subscribe(
    rxcpp::util::apply_to([progressWidget](const std::string& serviceName,
                            const vtkRemoteObjectProvider::vtkProgressItem& item) {
      vtkLogF(INFO, "progress: ");
      if (item.Progress == 100)
      {
        progressWidget->enableProgress(false);
      }
      else
      {
        progressWidget->enableProgress(true);
        progressWidget->setProgress(QString::fromStdString(item.Message), item.Progress);
      }
    }));
}

//-----------------------------------------------------------------------------
TestBasicQtAppMainWindow::TestBasicQtAppMainWindow()
  : Internals(new TestBasicQtAppMainWindow::QInternals())
{
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);

  auto* progressWidget = new pqProgressWidget();
  internals.Ui.statusbar->addPermanentWidget(progressWidget, /*stretch*/ 0);

  pqQVTKWidget* qtRenderWidget = new pqQVTKWidget(this);
  internals.setupSession(qtRenderWidget, progressWidget);
  this->setCentralWidget(qtRenderWidget);
}

//-----------------------------------------------------------------------------
TestBasicQtAppMainWindow::~TestBasicQtAppMainWindow() = default;
