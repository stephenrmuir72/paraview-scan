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

#include "pqFileDialog.h"
#include "pqFiltersMenuReaction.h"
#include "pqInterfaceTracker.h"
#include "pqProgressManager.h"
#include "pqProgressWidget.h"
#include "pqProxy.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSourcesMenuReaction.h"
#include "pqStandardPropertyWidgetInterface.h"
#include "vtkCamera.h"
#include "vtkClientSession.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGUIApplication.h"
#include "vtkSMApplyController.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <vtk_rxqt.h>
// clang-format off
#include VTK_REMOTING_RXQT(rxqt.hpp)
// clang-format on

#include <QVBoxLayout>

class TestBasicQtAppMainWindow::QInternals
{
public:
  Ui::TestBasicQtAppMainWindow Ui;
  QTimer SpinTimer;
  bool StateRestored{ false };
  vtkTypeUInt32 SessionId{ 0 };
  vtkSMViewProxy* setupSession(vtkTypeUInt32 sessionId);
  void openData(const QString& fileName);
  void saveState(const QString& fileName);
};

void TestBasicQtAppMainWindow::QInternals::openData(const QString& fileName)
{
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto* session = pvapp->GetSession(this->SessionId);
  auto* pxm = session->GetProxyManager();

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  vtkLogF(INFO, "openData (%s)", qPrintable(fileName));

  auto reader =
    vtk::TakeSmartPointer(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "IOSSReader")));
  vtkSMApplyController::MarkShowOnApply(reader);

  controller->PreInitializeProxy(reader);
  vtkSMPropertyHelper(reader, "FileName").Set(fileName.toStdString().c_str());
  reader->UpdateVTKObjects();
  controller->PostInitializeProxy(reader);
  controller->RegisterPipelineProxy(reader);
}
void TestBasicQtAppMainWindow::QInternals::saveState(const QString& fileName)
{
  vtkLogF(INFO, "saving state to %s", qPrintable(fileName));
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto* session = pvapp->GetSession(this->SessionId);
  auto* pxm = session->GetProxyManager();
  if (!pxm->SaveXMLState(fileName.toStdString().c_str()))
  {
    vtkLogF(ERROR, "Error saving state");
  }
}

vtkSMViewProxy* TestBasicQtAppMainWindow::QInternals::setupSession(vtkTypeUInt32 sessionId)
{
  this->SessionId = sessionId;

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto* tracker = pvapp->GetInterfaceTracker();
  tracker->addInterface(new pqStandardPropertyWidgetInterface(tracker));
  auto* session = pvapp->GetSession(this->SessionId);
  auto* pxm = session->GetProxyManager();
  auto* smmodel = pvapp->GetServerManagerModel();
  pqServer* pqserver = smmodel->findServer(this->SessionId);

  // initialize the session.
  controller->InitializeSession(session);

  auto sphere =
    vtk::TakeSmartPointer(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "SphereSource")));
  controller->PreInitializeProxy(sphere);
  vtkSMPropertyHelper(sphere, "PhiResolution").Set(800);
  vtkSMPropertyHelper(sphere, "ThetaResolution").Set(800);
  sphere->UpdateVTKObjects();
  controller->PostInitializeProxy(sphere);
  controller->RegisterPipelineProxy(sphere);

  auto shrink =
    vtk::TakeSmartPointer(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "ShrinkFilter")));
  controller->PreInitializeProxy(shrink);
  vtkSMPropertyHelper(shrink, "Input").Set(sphere, 0);
  shrink->UpdateVTKObjects();
  controller->PostInitializeProxy(shrink);
  controller->RegisterPipelineProxy(shrink);

  auto view =
    vtk::TakeSmartPointer(vtkSMViewProxy::SafeDownCast(pxm->NewProxy("views", "RenderView")));
  controller->InitializeProxy(view);
  controller->RegisterViewProxy(view);
  controller->AssignViewToLayout(view);
  controller->Show(shrink, 0, view, "GeometryRepresentation");

  // Update the view.
  view->Update().subscribe([view](bool) { view->ResetCameraUsingVisiblePropBounds(); });
  return view;
}

//-----------------------------------------------------------------------------
TestBasicQtAppMainWindow::TestBasicQtAppMainWindow()
  : Internals(new TestBasicQtAppMainWindow::QInternals())
{
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);
  internals.SpinTimer.setSingleShot(false);

  auto* sourceMenuMgr = new pqProxyGroupMenuManager(internals.Ui.menu_Sources, "ParaViewSources");
  sourceMenuMgr->loadConfiguration(":/pqApplicationComponents/Menus/Sources.xml");
  sourceMenuMgr->addProxyDefinitionUpdateListener("sources");
  sourceMenuMgr->setRecentlyUsedMenuSize(10);
  new pqSourcesMenuReaction(sourceMenuMgr);

  auto* filtersMenuMgr = new pqProxyGroupMenuManager(internals.Ui.menu_Filters, "ParaViewFilters");
  filtersMenuMgr->loadConfiguration(":/pqApplicationComponents/Menus/Filters.xml");
  filtersMenuMgr->addProxyDefinitionUpdateListener("filters");
  filtersMenuMgr->setRecentlyUsedMenuSize(10);
  new pqFiltersMenuReaction(filtersMenuMgr);

  auto* progressWidget = new pqProgressWidget();
  internals.Ui.statusbar->addPermanentWidget(progressWidget, /*stretch*/ 0);

  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto* pgm = pvapp->GetProgressManager();
  QObject::connect(pgm, SIGNAL(enableProgress(bool)), progressWidget, SLOT(enableProgress(bool)));
  QObject::connect(pgm, SIGNAL(progress(const QString&, int)), progressWidget,
    SLOT(setProgress(const QString&, int)));

  QObject::connect(internals.Ui.actionSpin, &QAction::toggled, [&internals](bool checked) {
    if (checked)
    {
      internals.SpinTimer.start(10);
    }
    else
    {
      internals.SpinTimer.stop();
    }
  });

  // Hookup file-open dialog
  QObject::connect(
    internals.Ui.actionOpenFile, &QAction::triggered, [pvapp, this, &internals](bool /*unused*/) {
      auto* smmodel = pvapp->GetServerManagerModel();
      if (auto* pqserver = smmodel->findServer(internals.SessionId))
      {
        pqFileDialog dialog(pqserver, this);
        if (pqFileDialog::Accepted != dialog.exec())
        {
          vtkLogF(ERROR, "Selected file was not accepted");
        }
        else
        {
          QStringList fileList = dialog.getSelectedFiles();
          if (!fileList.empty())
          {
            internals.openData(fileList.front());
          }
        }
      }
    });

  QObject::connect(
    internals.Ui.actionSaveState, &QAction::triggered, [pvapp, this, &internals](bool /*unused*/) {
      auto* smmodel = pvapp->GetServerManagerModel();
      if (auto* pqserver = smmodel->findServer(internals.SessionId))
      {
        QString file = pqFileDialog::getSaveFileName(
          pqserver, this->Internals->Ui.dockWidget, "Save State as...");
        if (!file.isEmpty())
        {
          internals.saveState(file);
        }
      }
    });
}

//-----------------------------------------------------------------------------
TestBasicQtAppMainWindow::~TestBasicQtAppMainWindow() = default;

//-----------------------------------------------------------------------------
void TestBasicQtAppMainWindow::showEvent(QShowEvent* evt)
{
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto& internals = (*this->Internals);
  if (!internals.StateRestored)
  {
    internals.StateRestored = true;
    pvapp->GetSettings()->restoreState("mainWindow", *this);
  }
}

//-----------------------------------------------------------------------------
void TestBasicQtAppMainWindow::closeEvent(QCloseEvent* evt)
{
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  pvapp->GetSettings()->saveState(*this, "mainWindow");
  this->QMainWindow::closeEvent(evt);
}

//-----------------------------------------------------------------------------
void TestBasicQtAppMainWindow::connect(const QString& url)
{
  auto& internals = (*this->Internals);
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto observable =
    url.isEmpty() ? pvapp->CreateBuiltinSession() : pvapp->CreateRemoteSession(qUtf8Printable(url));
  observable.subscribe([&internals](vtkTypeUInt32 id) {
    auto* view = internals.setupSession(id);
    QObject::connect(&internals.SpinTimer, &QTimer::timeout, [view]() {
      view->GetCamera()->Azimuth(1);
      view->StillRender();
    });
  });
}
