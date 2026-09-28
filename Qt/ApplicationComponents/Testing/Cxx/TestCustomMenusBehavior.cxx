// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Tests pqCustomMenusBehavior, which adds menus and toolbars described in plugin
// XML configurations.

#include "pqActiveObjects.h"
#include "pqCustomMenusBehavior.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"

#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QThread>
#include <QToolBar>
#include <QToolButton>
#include <QtDebug>

#include <iostream>

namespace
{
const char* FirstXML = R"(
<ServerManagerConfiguration>
  <CustomMenus>
    <Menu name="MyTools" label="&amp;My Tools" show_in_toolbar="1">
      <Proxy group="sources" name="RTAnalyticSource" />
      <Proxy group="filters" name="ShrinkFilter" label="Shrink It" />
      <Separator />
      <Script label="Create Sphere" name="CreateSphere">
        <![CDATA[
        from paraview.simple import Sphere
        if True:
            Sphere(registrationName="SphereFromScript")
        ]]>
      </Script>
      <Menu label="More">
        <Proxy group="filters" name="Cut" />
      </Menu>
    </Menu>
    <Menu name="ToolbarOnly" show_in_menubar="0" show_in_toolbar="1">
      <Proxy group="sources" name="SphereSource" />
    </Menu>
  </CustomMenus>
</ServerManagerConfiguration>)";

// adds items to the menus defined above, as a separate plugin may.
const char* SecondXML = R"(
<CustomMenus>
  <Menu label="My Tools">
    <Menu label="More">
      <Proxy group="filters" name="Contour" />
    </Menu>
  </Menu>
</CustomMenus>)";

#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Failed check at line " << __LINE__ << ": " #condition << std::endl;              \
    return false;                                                                                  \
  }

QMenu* findMenu(QMenuBar* menubar, const QString& name)
{
  for (QAction* action : menubar->actions())
  {
    if (action->menu() && action->menu()->objectName() == name)
    {
      return action->menu();
    }
  }
  return nullptr;
}

QAction* findAction(const QList<QAction*>& actions, const QString& text)
{
  for (QAction* action : actions)
  {
    if (action->text() == text)
    {
      return action;
    }
  }
  return nullptr;
}

/**
 * Mimics pqApplyBehavior applying a newly created source: the pipeline is
 * updated, as the data is shown, before the source is marked as unmodified.
 */
void apply(pqPipelineSource* source)
{
  pqActiveObjects::instance().setActiveSource(source);
  source->updatePipeline();
  source->setModifiedState(pqProxy::UNMODIFIED);
  Q_EMIT pqApplicationCore::instance() -> forceFilterMenuRefresh();
}

/**
 * Processes events until the action is enabled, or a timeout.
 */
bool waitUntilEnabled(QAction* action)
{
  QElapsedTimer timer;
  timer.start();
  while (!action->isEnabled() && timer.elapsed() < 5000)
  {
    QCoreApplication::processEvents();
    QThread::msleep(10);
  }
  return action->isEnabled();
}

bool doTest(QMainWindow& window)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqApplicationCore::instance()->loadConfigurationXML(FirstXML);
  pqApplicationCore::instance()->loadConfigurationXML(SecondXML);

  // the menu is added before the "Help" menu.
  QMenuBar* menubar = window.menuBar();
  QMenu* menu = findMenu(menubar, "MyTools");
  CHECK(menu != nullptr);
  CHECK(menu->title() == "&My Tools");
  CHECK(menubar->actions().size() == 2 && menubar->actions()[0] == menu->menuAction());
  CHECK(findMenu(menubar, "ToolbarOnly") == nullptr);

  // items, with default and custom labels.
  QList<QAction*> actions = menu->actions();
  CHECK(actions.size() == 5);
  CHECK(actions[0]->text() == "Wavelet");
  CHECK(actions[1]->text() == "Shrink It");
  CHECK(actions[2]->isSeparator());
  CHECK(actions[3]->text() == "Create Sphere");
  CHECK(actions[4]->menu() != nullptr);

  // the sub-menu has items from both configurations.
  QMenu* subMenu = actions[4]->menu();
  CHECK(subMenu->title() == "More");
  CHECK(subMenu->actions().size() == 2);
  CHECK(subMenu->actions()[0]->text() == "Slice");
  CHECK(subMenu->actions()[1]->text() == "Contour");

  // the toolbar has the same items, with the sub-menu as a drop-down button.
  auto toolbar = window.findChild<QToolBar*>("MyTools");
  CHECK(toolbar != nullptr);
  CHECK(toolbar->windowTitle() == "My Tools");
  QList<QAction*> toolbarActions = toolbar->actions();
  CHECK(toolbarActions.size() == actions.size());
  for (int cc = 0; cc < actions.size(); ++cc)
  {
    // separators are distinct actions.
    CHECK(actions[cc]->isSeparator() ? toolbarActions[cc]->isSeparator()
                                     : toolbarActions[cc] == actions[cc]);
  }
  auto button = qobject_cast<QToolButton*>(toolbar->widgetForAction(subMenu->menuAction()));
  CHECK(button && button->popupMode() == QToolButton::InstantPopup);

  auto toolbarOnly = window.findChild<QToolBar*>("ToolbarOnly");
  CHECK(toolbarOnly != nullptr);
  CHECK(toolbarOnly->actions().size() == 1 && toolbarOnly->actions()[0]->text() == "Sphere");

  // filters require an input.
  QAction* shrink = actions[1];
  CHECK(actions[0]->isEnabled());
  CHECK(!shrink->isEnabled());

  // create a source using the menu.
  const int numSources = smmodel->findItems<pqPipelineSource*>().size();
  actions[0]->trigger();
  QList<pqPipelineSource*> sources = smmodel->findItems<pqPipelineSource*>();
  CHECK(sources.size() == numSources + 1);
  pqPipelineSource* wavelet = sources.back();
  CHECK(QString(wavelet->getProxy()->GetXMLName()) == "RTAnalyticSource");
  apply(wavelet);

  // create a filter using the menu, once enabled for the source.
  CHECK(waitUntilEnabled(shrink));
  shrink->trigger();
  sources = smmodel->findItems<pqPipelineSource*>();
  CHECK(sources.size() == numSources + 2);
  CHECK(QString(sources.back()->getProxy()->GetXMLName()) == "ShrinkFilter");
  auto shrinkFilter = qobject_cast<pqPipelineFilter*>(sources.back());
  CHECK(shrinkFilter && shrinkFilter->getInput(0) == wavelet);

#if VTK_MODULE_ENABLE_ParaView_pqPython
  // run the inline script.
  actions[3]->trigger();
  CHECK(smmodel->findItem<pqPipelineSource*>("SphereFromScript") != nullptr);
#else
  CHECK(!actions[3]->isEnabled());
#endif

  // a sub-menu first added to the menu only, is added to the toolbar when
  // requested later.
  pqApplicationCore::instance()->loadConfigurationXML(
    R"(<CustomMenus><Menu label="My Tools"><Menu label="Extra">
         <Proxy group="sources" name="SphereSource" /></Menu></Menu></CustomMenus>)");
  QAction* extra = menu->actions().back();
  CHECK(extra->menu() && extra->menu()->title() == "Extra");
  CHECK(!toolbar->actions().contains(extra));
  pqApplicationCore::instance()->loadConfigurationXML(
    R"(<CustomMenus><Menu label="My Tools" show_in_toolbar="1"><Menu label="Extra">
         <Proxy group="sources" name="ConeSource" /></Menu></Menu></CustomMenus>)");
  CHECK(menu->actions().back() == extra);
  CHECK(toolbar->actions().back() == extra);
  CHECK(extra->menu()->actions().size() == 2);
  return true;
}
}

extern int TestCustomMenusBehavior(int argc, char* argv[])
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);

  QMainWindow window;
  window.menuBar()->addMenu("&Help");
  new pqCustomMenusBehavior(&window);

  pqObjectBuilder* builder = appCore.getObjectBuilder();
  pqServer* server = builder->createServer(pqServerResource(vtkSMSession::GetBuiltinName()));
  pqActiveObjects::instance().setActiveServer(server);

  const bool success = doTest(window);
  builder->destroyAllProxies(server);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
