/*=========================================================================

   Program: ParaView
   Module:    ParaViewMainWindow.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#if PARAVIEW_USE_PYTHON
#include "pvpythonmodules.h"
#endif

#include "ParaViewMainWindow.h"
#include "ui_ParaViewMainWindow.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqServerResource.h"
// #include "pqDeleteReaction.h"
// #include "pqMainWindowEventManager.h"
#include "pqObjectBuilder.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"
#include "pqSettings.h"
#include "pqTimer.h"
#include "pqWelcomeDialog.h"
#include "vtkCommand.h"
#include "vtkPVGUIApplication.h"
#include "vtkPVGUIOptions.h"
#include "vtkPVGeneralSettings.h"
#include "vtksys/SystemTools.hxx"

#include "pqQtConfig.h"
#ifdef PARAVIEW_USE_QTHELP
// #include "pqHelpReaction.h"
#endif

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMessageBox>
#include <QTextCodec>
#include <QtDebug>

#if PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION
#include "ParaViewDocumentationInitializer.h"
#endif

#if PARAVIEW_USE_MATERIALEDITOR
#include "pqMaterialEditor.h"
#endif

#if PARAVIEW_USE_PYTHON
#include "pqPythonDebugLeaksView.h"
#include "pqPythonShell.h"
typedef pqPythonDebugLeaksView DebugLeaksViewType;
#else
#include "vtkQtDebugLeaksView.h"
typedef vtkQtDebugLeaksView DebugLeaksViewType;
#endif

class ParaViewMainWindow::pqInternals : public Ui::pqClientMainWindow
{
public:
  bool FirstShow;
  int CurrentGUIFontSize;
  QFont DefaultApplicationFont; // will be initialized to default app font in constructor.
  pqTimer UpdateFontSizeTimer;
  pqInternals()
    : FirstShow(true)
    , CurrentGUIFontSize(0)
  {
    this->UpdateFontSizeTimer.setInterval(0);
    this->UpdateFontSizeTimer.setSingleShot(true);
  }
};

//-----------------------------------------------------------------------------
ParaViewMainWindow::ParaViewMainWindow()
{
  // the debug leaks view should be constructed as early as possible
  // so that it can monitor vtk objects created at application startup.
  DebugLeaksViewType* leaksView = nullptr;
  if (vtksys::SystemTools::GetEnv("PV_DEBUG_LEAKS_VIEW"))
  {
    leaksView = new DebugLeaksViewType(this);
    leaksView->setWindowFlags(Qt::Window);
    leaksView->show();
  }

#if PARAVIEW_USE_PYTHON
  pvpythonmodules_load();
#endif

#if PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION
  // init the ParaView embedded documentation.
  paraview_documentation_initialize();
#endif

  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  this->Internals->outputWidgetDock->hide();
  this->Internals->pythonShellDock->hide();
#if PARAVIEW_USE_PYTHON
  pqPythonShell* shell = new pqPythonShell(this);
  shell->setObjectName("pythonShell");
  this->Internals->pythonShellDock->setWidget(shell);
  if (leaksView)
  {
    leaksView->setShell(shell);
  }
#endif

  // show output widget if we received an error message.
  this->connect(this->Internals->outputWidget, SIGNAL(messageDisplayed(const QString&, int)),
    SLOT(handleMessage(const QString&, int)));

  // Setup default GUI layout.
  this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->informationDock);

  // update UI when font size changes.
  vtkPVGeneralSettings* gsSettings = vtkPVGeneralSettings::GetInstance();
  pqCoreUtilities::connect(
    gsSettings, vtkCommand::ModifiedEvent, &this->Internals->UpdateFontSizeTimer, SLOT(start()));
  this->connect(&this->Internals->UpdateFontSizeTimer, SIGNAL(timeout()), SLOT(updateFontSize()));

  this->Internals->propertiesDock->show();
  this->Internals->propertiesDock->raise();

  // Enable help from the properties panel.
  QObject::connect(this->Internals->propertiesPanel,
    SIGNAL(helpRequested(const QString&, const QString&)), this,
    SLOT(showHelpForProxy(const QString&, const QString&)));

#if 0 // FIXME:ASYNC
  /// hook delete to pqDeleteReaction.
  QAction* tempDeleteAction = new QAction(this);
  pqDeleteReaction* handler = new pqDeleteReaction(tempDeleteAction);
  handler->connect(this->Internals->propertiesPanel, SIGNAL(deleteRequested(pqProxy*)),
    SLOT(deleteSource(pqProxy*)));

  // setup color editor
  /// Provide access to the color-editor panel for the application.
  pqApplicationCore::instance()->registerManager(
    "COLOR_EDITOR_PANEL", this->Internals->colorMapEditorDock);

  // Provide access to the find data panel for the application.
  pqApplicationCore::instance()->registerManager("FIND_DATA_PANEL", this->Internals->findDataDock);
#endif

  // Populate application menus with actions.
  pqParaViewMenuBuilders::buildFileMenu(*this->Internals->menu_File);
  pqParaViewMenuBuilders::buildEditMenu(
    *this->Internals->menu_Edit, this->Internals->propertiesPanel);

  pqParaViewMenuBuilders::buildToolbars(*this);

  // Populate filters menu.
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Populate sources menu.
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);

  // Populate extractors menu.
  pqParaViewMenuBuilders::buildExtractorsMenu(*this->Internals->menuExtractors, this);

  // Populate Tools menu.
  pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools);

  // Populate Catalyst menu.
  pqParaViewMenuBuilders::buildCatalystMenu(*this->Internals->menu_Catalyst);

  // setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(
    *this->Internals->pipelineBrowser->contextMenu());

  // Setup the View menu. This must be setup after all toolbars and dockwidgets
  // have been created.
  pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menu_View, *this);

  // Setup the menu to show macros.
  pqParaViewMenuBuilders::buildMacrosMenu(*this->Internals->menu_Macros);

  // Setup the help menu.
  pqParaViewMenuBuilders::buildHelpMenu(*this->Internals->menu_Help);

  // Final step, define application behaviors. Since we want all ParaView
  // behaviors, we use this convenience method.

  // UsageLoggingBehavior needs to explicitly enabled for ParaView since it's
  // disabled by default.
  pqParaViewBehaviors::setEnableUsageLoggingBehavior(true);
  new pqParaViewBehaviors(this, this);
}

//-----------------------------------------------------------------------------
ParaViewMainWindow::~ParaViewMainWindow()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::showHelpForProxy(const QString& groupname, const QString& proxyname)
{
#ifdef PARAVIEW_USE_QTHELP
#if 0 // FIXME: ASYNC
  pqHelpReaction::showProxyHelp(groupname, proxyname);
#endif
#endif
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::showEvent(QShowEvent* evt)
{
  this->Superclass::showEvent(evt);

  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto& internals = (*this->Internals);
  if (internals.FirstShow)
  {
    internals.FirstShow = false;
    pvapp->GetSettings()->restoreState("mainWindow", *this);
    this->startup();
  }
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::closeEvent(QCloseEvent* evt)
{
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  pvapp->GetSettings()->saveState(*this, "mainWindow");
  this->Superclass::closeEvent(evt);
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::showWelcomeDialog()
{
  pqWelcomeDialog dialog(this);
  dialog.exec();
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::updateFontSize()
{
  auto& internals = *this->Internals;
  vtkPVGeneralSettings* gsSettings = vtkPVGeneralSettings::GetInstance();
  int fontSize = internals.DefaultApplicationFont.pointSize();
  if (gsSettings->GetGUIOverrideFont() && gsSettings->GetGUIFontSize() > 0)
  {
    fontSize = gsSettings->GetGUIFontSize();
  }
  if (fontSize <= 0)
  {
    qDebug() << "Ignoring invalid font size: " << fontSize;
    return;
  }

  if (internals.CurrentGUIFontSize != fontSize)
  {
    QFont newFont = this->font();
    newFont.setPointSize(fontSize);
    this->setFont(newFont);
    this->Internals->CurrentGUIFontSize = fontSize;
  }

// Console font size
#if PARAVIEW_USE_PYTHON
  pqPythonShell* shell = qobject_cast<pqPythonShell*>(this->Internals->pythonShellDock->widget());
  shell->setFontSize(fontSize);
#endif
  pqOutputWidget* outputWidget =
    qobject_cast<pqOutputWidget*>(this->Internals->outputWidgetDock->widget());
  outputWidget->setFontSize(fontSize);
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::handleMessage(const QString&, int type)
{
  QDockWidget* dock = this->Internals->outputWidgetDock;
  if (!dock->isVisible() && (type == QtCriticalMsg || type == QtFatalMsg || type == QtWarningMsg))
  {
    // if dock is not visible, we always pop it up as a floating dialog. This
    // avoids causing re-renders which may cause more errors and more confusion.
    QRect rectApp = this->geometry();

    QRect rectDock(
      QPoint(0, 0), QSize(static_cast<int>(rectApp.width() * 0.4), dock->sizeHint().height()));
    rectDock.moveCenter(
      QPoint(rectApp.center().x(), rectApp.bottom() - dock->sizeHint().height() / 2));
    dock->setFloating(true);
    dock->setGeometry(rectDock);
    dock->show();
  }
  if (dock->isVisible())
  {
    dock->raise();
  }
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::startup()
{
  auto* pvapp = vtkPVGUIApplication::GetInstance();
  auto* guiOptions = pvapp->GetOptions();
  auto* settings = pvapp->GetSettings();

  // show welcome dialog.
  if (!guiOptions->GetDisableRegistry())
  {
    if (settings->value("GeneralSettings.ShowWelcomeDialog", true).toBool())
    {
      pqTimer::singleShot(1000, this, SLOT(showWelcomeDialog()));
    }
  }

  // update font size.
  if (!guiOptions->GetDisableRegistry())
  {
    this->updateFontSize();
  }

  // connect to default server.
  pvapp->GetObjectBuilder()->createServer(pqServerResource("builtin:"));
}
