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
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkClientSession.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGUIApplication.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <QVBoxLayout>

class TestBasicQtAppMainWindow::QInternals
{
public:
  Ui::TestBasicQtAppMainWindow Ui;
  void setupSession(QWidget* centralWidget, pqProgressWidget* progressWidget);
};

void TestBasicQtAppMainWindow::QInternals::setupSession(
  QWidget* centralWidget, pqProgressWidget* progressWidget)
{
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto sessionId = pvapp->CreateBuiltinSession();
  auto* session = pvapp->GetSession(sessionId);
  auto* pxm = session->GetProxyManager();
  auto* smmodel = pvapp->GetServerManagerModel();

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
  pxm->RegisterProxy("views", viewProxy);

  auto* pqview = smmodel->findItem<pqView*>(viewProxy);
  Q_ASSERT(pqview);
  centralWidget->layout()->addWidget(pqview->widget());

  viewProxy->InvokeCommand("Update");

  auto observable = session->GetProgressObservable().subscribe(
    rxcpp::util::apply_to([progressWidget](const std::string& serviceName,
                            const vtkRemoteObjectProvider::vtkProgressItem& item) {
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

  QObject::connect(this->Ui.propertiesPanel, QOverload<pqProxy*>::of(&pqPropertiesPanel::applied),
    [](pqProxy* proxy) { proxy->setModifiedState(pqProxy::UNMODIFIED); });

  QObject::connect(this->Ui.propertiesPanel, QOverload<>::of(&pqPropertiesPanel::applied),
    [viewProxy]() { viewProxy->InvokeCommand("Update"); });
}

//-----------------------------------------------------------------------------
TestBasicQtAppMainWindow::TestBasicQtAppMainWindow()
  : Internals(new TestBasicQtAppMainWindow::QInternals())
{
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);

  auto* progressWidget = new pqProgressWidget();
  internals.Ui.statusbar->addPermanentWidget(progressWidget, /*stretch*/ 0);

  QWidget* centerWidget = new QWidget(this);
  new QVBoxLayout(centerWidget);
  internals.setupSession(centerWidget, progressWidget);
  this->setCentralWidget(centerWidget);
}

//-----------------------------------------------------------------------------
TestBasicQtAppMainWindow::~TestBasicQtAppMainWindow() = default;
