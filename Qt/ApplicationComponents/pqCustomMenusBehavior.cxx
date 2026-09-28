// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCustomMenusBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqFiltersMenuReaction.h"
#include "pqPluginManager.h"
#include "pqProxyAction.h"
#include "pqServer.h"
#include "pqSourcesMenuReaction.h"
#include "pqWidgetUtilities.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"

#if VTK_MODULE_ENABLE_ParaView_pqPython
#include "pqPVApplicationCore.h"
#include "pqPythonManager.h"
#endif

#include <QAction>
#include <QCoreApplication>
#include <QIcon>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>

#include <algorithm>
#include <cstring>

namespace
{
const char* DefaultLabelProperty = "pqCustomMenusBehavior_DefaultLabel";

bool hasName(vtkPVXMLElement* element, const char* name)
{
  return element && element->GetName() && strcmp(element->GetName(), name) == 0;
}

QString attribute(vtkPVXMLElement* element, const char* name)
{
  const char* value = element->GetAttribute(name);
  return value ? QString::fromUtf8(value) : QString();
}

QString withoutMnemonic(const QString& text)
{
  return QString(text).remove('&');
}

/**
 * Returns the name and label for a Menu element. Either may be derived from
 * the other when not specified.
 */
std::pair<QString, QString> menuNameAndLabel(vtkPVXMLElement* menuXML)
{
  QString name = attribute(menuXML, "name");
  QString label = attribute(menuXML, "label");
  if (name.isEmpty())
  {
    name = withoutMnemonic(label);
  }
  if (label.isEmpty())
  {
    label = name;
  }
  return { name, label };
}

/**
 * Returns true if the menu matches the given name or label.
 */
bool matches(QMenu* menu, const QString& name, const QString& label)
{
  return menu &&
    (menu->objectName() == name || withoutMnemonic(menu->title()) == withoutMnemonic(label));
}

QAction* findHelpMenuAction(QMenuBar* menubar)
{
  for (QAction* action : menubar->actions())
  {
    if (withoutMnemonic(action->text()).toLower() == "help")
    {
      return action;
    }
  }
  return nullptr;
}

/**
 * Removes leading and trailing blank lines and the indentation common to all
 * lines, so that Python code can be indented to match the XML it's part of.
 */
QString dedent(const QString& text)
{
  QStringList lines = text.split('\n');
  while (!lines.isEmpty() && lines.front().trimmed().isEmpty())
  {
    lines.pop_front();
  }
  while (!lines.isEmpty() && lines.back().trimmed().isEmpty())
  {
    lines.pop_back();
  }

  auto indentation = [](const QString& line)
  {
    int count = 0;
    while (count < line.size() && (line[count] == ' ' || line[count] == '\t'))
    {
      ++count;
    }
    return count;
  };

  int common = -1;
  for (const QString& line : lines)
  {
    if (!line.trimmed().isEmpty())
    {
      common = common < 0 ? indentation(line) : std::min(common, indentation(line));
    }
  }
  for (QString& line : lines)
  {
    line.remove(0, std::min(std::max(common, 0), indentation(line)));
  }
  return lines.join('\n') + '\n';
}

vtkSMProxy* findPrototype(const QString& group, const QString& name)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  return server
    ? server->proxyManager()->GetPrototypeProxy(group.toUtf8().data(), name.toUtf8().data())
    : nullptr;
}

QString proxyLabel(vtkSMProxy* prototype)
{
  return prototype && prototype->GetXMLLabel()
    ? QCoreApplication::translate("ServerManagerXML", prototype->GetXMLLabel())
    : QString();
}
}

//-----------------------------------------------------------------------------
pqCustomMenusBehavior::pqCustomMenusBehavior(QMainWindow* mainWindow)
  : Superclass(mainWindow)
  , MainWindow(mainWindow)
{
  QObject::connect(pqApplicationCore::instance(), &pqApplicationCore::loadXML, this,
    &pqCustomMenusBehavior::loadConfiguration);

  // Like the Filters menu, update the enabled state after a short delay so that
  // changes are complete e.g. a new source is only marked as unmodified after
  // its data is updated when applied.
  this->UpdateTimer.setInterval(10);
  this->UpdateTimer.setSingleShot(true);
  QObject::connect(
    &this->UpdateTimer, &QTimer::timeout, this, &pqCustomMenusBehavior::updateEnableState);
  auto startTimer = [this]() { this->UpdateTimer.start(); };

  auto& activeObjects = pqActiveObjects::instance();
  QObject::connect(&activeObjects, &pqActiveObjects::serverChanged, this, startTimer);
  QObject::connect(&activeObjects, &pqActiveObjects::portChanged, this, startTimer);
  QObject::connect(&activeObjects, &pqActiveObjects::selectionChanged, this, startTimer);
  QObject::connect(&activeObjects, &pqActiveObjects::dataUpdated, this, startTimer);

  pqApplicationCore* core = pqApplicationCore::instance();
  QObject::connect(core, &pqApplicationCore::forceFilterMenuRefresh, this, startTimer);
  QObject::connect(core, &pqApplicationCore::updateMasterEnableState, this, startTimer);
  QObject::connect(core->getPluginManager(), &pqPluginManager::pluginsUpdated, this, startTimer);
}

//-----------------------------------------------------------------------------
pqCustomMenusBehavior::~pqCustomMenusBehavior() = default;

//-----------------------------------------------------------------------------
void pqCustomMenusBehavior::loadConfiguration(vtkPVXMLElement* root)
{
  if (!root || !this->MainWindow)
  {
    return;
  }

  auto addMenus = [this](vtkPVXMLElement* customMenus)
  {
    for (unsigned int cc = 0; cc < customMenus->GetNumberOfNestedElements(); ++cc)
    {
      vtkPVXMLElement* child = customMenus->GetNestedElement(cc);
      if (hasName(child, "Menu"))
      {
        this->addTopLevelMenu(child);
      }
    }
  };

  if (hasName(root, "CustomMenus"))
  {
    addMenus(root);
  }
  else
  {
    for (unsigned int cc = 0; cc < root->GetNumberOfNestedElements(); ++cc)
    {
      vtkPVXMLElement* child = root->GetNestedElement(cc);
      if (hasName(child, "CustomMenus"))
      {
        addMenus(child);
      }
    }
  }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqCustomMenusBehavior::addTopLevelMenu(vtkPVXMLElement* menuXML)
{
  const auto nameAndLabel = ::menuNameAndLabel(menuXML);
  if (nameAndLabel.first.isEmpty())
  {
    qWarning("A 'Menu' in 'CustomMenus' requires a 'name' or a 'label'.");
    return;
  }

  int showInMenubar = 1;
  menuXML->GetScalarAttribute("show_in_menubar", &showInMenubar);
  int showInToolbar = 0;
  menuXML->GetScalarAttribute("show_in_toolbar", &showInToolbar);

  QMenu* menu =
    showInMenubar ? this->findOrCreateMenu(nameAndLabel.first, nameAndLabel.second) : nullptr;
  QToolBar* toolbar =
    showInToolbar ? this->findOrCreateToolBar(nameAndLabel.first, nameAndLabel.second) : nullptr;
  if (menu || toolbar)
  {
    this->addItems(menuXML, menu, toolbar);
  }
}

//-----------------------------------------------------------------------------
QMenu* pqCustomMenusBehavior::findOrCreateMenu(const QString& name, const QString& label)
{
  QMenuBar* menubar = this->MainWindow->menuBar();
  for (QAction* action : menubar->actions())
  {
    if (::matches(action->menu(), name, label))
    {
      return action->menu();
    }
  }

  auto menu = new QMenu(label, this->MainWindow);
  menu->setObjectName(name);
  // insert new menus before the Help menu, if any.
  menubar->insertMenu(::findHelpMenuAction(menubar), menu);
  return menu;
}

//-----------------------------------------------------------------------------
QToolBar* pqCustomMenusBehavior::findOrCreateToolBar(const QString& name, const QString& label)
{
  const QString title = ::withoutMnemonic(label);
  for (QToolBar* toolbar : this->MainWindow->findChildren<QToolBar*>())
  {
    if (toolbar->objectName() == name || toolbar->windowTitle() == title)
    {
      return toolbar;
    }
  }

  auto toolbar = new QToolBar(title, this->MainWindow);
  toolbar->setObjectName(name);
  this->MainWindow->addToolBar(toolbar);
  return toolbar;
}

//-----------------------------------------------------------------------------
void pqCustomMenusBehavior::addItems(vtkPVXMLElement* menuXML, QMenu* menu, QToolBar* toolbar)
{
  auto addAction = [menu, toolbar](QAction* action)
  {
    if (!action)
    {
      return;
    }
    if (menu)
    {
      menu->addAction(action);
    }
    if (toolbar)
    {
      toolbar->addAction(action);
    }
  };

  for (unsigned int cc = 0; cc < menuXML->GetNumberOfNestedElements(); ++cc)
  {
    vtkPVXMLElement* itemXML = menuXML->GetNestedElement(cc);
    if (hasName(itemXML, "Proxy"))
    {
      addAction(this->createProxyAction(itemXML, this));
    }
    else if (hasName(itemXML, "Script"))
    {
      addAction(this->createScriptAction(itemXML, this));
    }
    else if (hasName(itemXML, "Separator"))
    {
      if (menu)
      {
        menu->addSeparator();
      }
      if (toolbar)
      {
        toolbar->addSeparator();
      }
    }
    else if (hasName(itemXML, "Menu"))
    {
      const auto nameAndLabel = ::menuNameAndLabel(itemXML);
      if (nameAndLabel.first.isEmpty())
      {
        qWarning("A 'Menu' in 'CustomMenus' requires a 'name' or a 'label'.");
        continue;
      }

      // sub-menus with the same name are merged, which lets several
      // configurations add items to the same sub-menu.
      QList<QAction*> existingActions = menu ? menu->actions() : toolbar->actions();
      auto iter = std::find_if(existingActions.begin(), existingActions.end(), [&](QAction* action)
        { return ::matches(action->menu(), nameAndLabel.first, nameAndLabel.second); });
      QMenu* subMenu = iter != existingActions.end() ? (*iter)->menu() : nullptr;
      if (!subMenu)
      {
        subMenu = new QMenu(nameAndLabel.second, this->MainWindow);
        subMenu->setObjectName(nameAndLabel.first);
        const QString icon = attribute(itemXML, "icon");
        if (!icon.isEmpty())
        {
          subMenu->setIcon(QIcon(icon));
        }
        if (menu)
        {
          menu->addAction(subMenu->menuAction());
        }
      }
      this->addItems(itemXML, subMenu, nullptr);

      if (toolbar && !toolbar->actions().contains(subMenu->menuAction()))
      {
        // show the sub-menu as a drop-down button.
        toolbar->addAction(subMenu->menuAction());
        if (auto button =
              qobject_cast<QToolButton*>(toolbar->widgetForAction(subMenu->menuAction())))
        {
          button->setPopupMode(QToolButton::InstantPopup);
        }
      }
    }
    else if (itemXML && itemXML->GetName())
    {
      qWarning() << "Unknown element in 'CustomMenus':" << itemXML->GetName();
    }
  }
}

//-----------------------------------------------------------------------------
QAction* pqCustomMenusBehavior::createProxyAction(vtkPVXMLElement* itemXML, QObject* parent)
{
  const QString group = attribute(itemXML, "group");
  const QString name = attribute(itemXML, "name");
  if (group != "filters" && group != "sources")
  {
    qWarning() << "'Proxy' in 'CustomMenus' requires 'group' to be 'filters' or 'sources', got"
               << group;
    return nullptr;
  }
  if (name.isEmpty())
  {
    qWarning("'Proxy' in 'CustomMenus' requires a 'name'.");
    return nullptr;
  }

  vtkSMProxy* prototype = ::findPrototype(group, name);
  QString label = attribute(itemXML, "label");
  const bool defaultLabel = label.isEmpty();
  if (defaultLabel)
  {
    label = ::proxyLabel(prototype);
  }
  QString icon = attribute(itemXML, "icon");
  if (icon.isEmpty() && prototype && prototype->GetHints())
  {
    vtkPVXMLElement* showInMenu = prototype->GetHints()->FindNestedElementByName("ShowInMenu");
    if (showInMenu && showInMenu->GetAttribute("icon"))
    {
      icon = QString::fromUtf8(showInMenu->GetAttribute("icon"));
    }
  }

  auto action = new QAction(label.isEmpty() ? name : label, parent);
  action->setObjectName(name);
  // this is used by pqProxyAction to determine the proxy for the action.
  action->setData(QStringList() << group << name);
  action->setProperty(DefaultLabelProperty, defaultLabel);
  if (!icon.isEmpty())
  {
    action->setIcon(QIcon(icon));
  }

  if (group == "filters")
  {
    QObject::connect(action, &QAction::triggered, action,
      [group, name]() { pqFiltersMenuReaction::createFilter(group, name); });
    this->FilterActions.push_back(action);
  }
  else
  {
    QObject::connect(action, &QAction::triggered, action,
      [group, name]() { pqSourcesMenuReaction::createSource(group, name); });
    this->SourceActions.push_back(action);
  }
  return action;
}

//-----------------------------------------------------------------------------
QAction* pqCustomMenusBehavior::createScriptAction(vtkPVXMLElement* itemXML, QObject* parent)
{
  const QString label = attribute(itemXML, "label");
  const QString filename = attribute(itemXML, "filename");
  const QString code = ::dedent(QString::fromUtf8(itemXML->GetCharacterData()));
  if (label.isEmpty())
  {
    qWarning("'Script' in 'CustomMenus' requires a 'label'.");
    return nullptr;
  }
  if (filename.isEmpty() && code.trimmed().isEmpty())
  {
    qWarning() << "'Script' in 'CustomMenus' requires either inline code or a 'filename':" << label;
    return nullptr;
  }

  auto action = new QAction(label, parent);
  const QString name = attribute(itemXML, "name");
  action->setObjectName(name.isEmpty() ? ::withoutMnemonic(label) : name);
  const QString icon = attribute(itemXML, "icon");
  if (!icon.isEmpty())
  {
    action->setIcon(QIcon(icon));
  }
  const QString tooltip = attribute(itemXML, "tooltip");
  if (!tooltip.isEmpty())
  {
    action->setToolTip(pqWidgetUtilities::formatTooltip(tooltip));
    action->setStatusTip(tooltip);
  }

#if VTK_MODULE_ENABLE_ParaView_pqPython
  QObject::connect(action, &QAction::triggered, action,
    [filename, code]()
    {
      auto core = pqPVApplicationCore::instance();
      pqPythonManager* manager = core ? core->pythonManager() : nullptr;
      if (!manager)
      {
        qCritical("No application wide python manager.");
        return;
      }
      if (!filename.isEmpty())
      {
        manager->executeScriptAndRender(filename);
      }
      else
      {
        manager->executeCode(code.toUtf8());
        pqApplicationCore::instance()->render();
      }
    });
  this->ScriptActions.push_back(action);
#else
  (void)filename;
  action->setEnabled(false);
  action->setStatusTip(tr("Requires Python support"));
#endif
  return action;
}

//-----------------------------------------------------------------------------
void pqCustomMenusBehavior::updateEnableState()
{
  pqServer* server = pqActiveObjects::instance().activeServer();

  QList<QAction*> filterActions;
  for (const auto& action : this->FilterActions)
  {
    if (action)
    {
      filterActions.push_back(action);
    }
  }

  // use the proxy labels, unless specified, once the proxy definitions are
  // available.
  QList<QAction*> proxyActions = filterActions;
  for (const auto& action : this->SourceActions)
  {
    if (action)
    {
      proxyActions.push_back(action);
    }
  }
  for (QAction* action : proxyActions)
  {
    if (action->property(DefaultLabelProperty).toBool())
    {
      const QString label = ::proxyLabel(pqProxyAction::GetProxyPrototype(action));
      if (!label.isEmpty())
      {
        action->setText(label);
        action->setProperty(DefaultLabelProperty, false);
      }
    }
    if (action->toolTip().isEmpty() || action->toolTip() == action->text())
    {
      const QString documentation = pqProxyAction::GetProxyDocumentation(action);
      if (!documentation.isEmpty())
      {
        action->setToolTip(pqWidgetUtilities::formatTooltip(documentation));
      }
    }
  }

  // filters are enabled based on the selected pipeline objects, like in the
  // Filters menu.
  pqProxyAction::updateActionsState(filterActions);

  const bool canCreateSources = server && server->isMaster();
  for (const auto& action : this->SourceActions)
  {
    if (action)
    {
      action->setEnabled(canCreateSources);
    }
  }
}
