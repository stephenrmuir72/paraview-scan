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

#include "pqProgressManager.h"
#include "pqProgressWidget.h"
#include "pqProxy.h"
#include "pqProxyGroupMenuManager.h"
#include "pqSourcesMenuReaction.h"
#include "vtkCamera.h"
#include "vtkClientSession.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGUIApplication.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
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
  vtkSMViewProxy* setupSession(pqProgressWidget* progressWidget);
};

vtkSMViewProxy* TestBasicQtAppMainWindow::QInternals::setupSession(pqProgressWidget* progressWidget)
{
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto sessionId = pvapp->CreateBuiltinSession();
  auto* session = pvapp->GetSession(sessionId);
  auto* pxm = session->GetProxyManager();
  auto* smmodel = pvapp->GetServerManagerModel();

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

  auto* pgm = pvapp->GetProgressManager();
  QObject::connect(pgm, SIGNAL(enableProgress(bool)), progressWidget, SLOT(enableProgress(bool)));
  QObject::connect(pgm, SIGNAL(progress(const QString&, int)), progressWidget,
    SLOT(setProgress(const QString&, int)));

  QObject::connect(this->Ui.propertiesPanel, QOverload<pqProxy*>::of(&pqPropertiesPanel::applied),
    [](pqProxy* proxy) { proxy->setModifiedState(pqProxy::UNMODIFIED); });

  QObject::connect(this->Ui.propertiesPanel, QOverload<>::of(&pqPropertiesPanel::applied),
    [view]() { view->InvokeCommand("Update"); });

  // Update the view.
  view->InvokeCommand("Update");
  return view;
}

//-----------------------------------------------------------------------------
TestBasicQtAppMainWindow::TestBasicQtAppMainWindow()
  : Internals(new TestBasicQtAppMainWindow::QInternals())
{
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);

  auto* sourceMenuMgr = new pqProxyGroupMenuManager(internals.Ui.menu_Sources, "ParaViewSources");
  sourceMenuMgr->loadConfiguration(":/pqApplicationComponents/Menus/Sources.xml");
  sourceMenuMgr->addProxyDefinitionUpdateListener("sources");
  sourceMenuMgr->setRecentlyUsedMenuSize(10);
  new pqSourcesMenuReaction(sourceMenuMgr);

#if 0
  auto* filtersMenuMgr = new pqProxyGroupMenuManager(internals.Ui.menu_Filters, "ParaViewFilters");
  filtersMenuMgr->loadConfiguration(":/pqApplicationComponents/Menus/Filters.xml");
  filtersMenuMgr->addProxyDefinitionUpdateListener("filters");
  filtersMenuMgr->setRecentlyUsedMenuSize(10);
#endif

  auto* progressWidget = new pqProgressWidget();
  internals.Ui.statusbar->addPermanentWidget(progressWidget, /*stretch*/ 0);

  auto* view = internals.setupSession(progressWidget);
  auto* timer = new QTimer(this);
  timer->setSingleShot(false);

  QObject::connect(timer, &QTimer::timeout, [view]() {
    view->GetCamera()->Azimuth(1);
    view->StillRender();
  });

  QObject::connect(internals.Ui.actionSpin, &QAction::toggled, [timer](bool checked) {
    if (checked)
    {
      timer->start(10);
    }
    else
    {
      timer->stop();
    }
  });
}

//-----------------------------------------------------------------------------
TestBasicQtAppMainWindow::~TestBasicQtAppMainWindow() = default;
