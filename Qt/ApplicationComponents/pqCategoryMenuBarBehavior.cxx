// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCategoryMenuBarBehavior.h"

#include "pqProxyCategory.h"
#include "pqProxyGroupMenuManager.h"

#include <QAction>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>

#include <cassert>

namespace
{
QAction* findHelpMenuAction(QMenuBar* menubar)
{
  for (QAction* action : menubar->actions())
  {
    if (action->text().remove('&').toLower() == "help")
    {
      return action;
    }
  }
  return nullptr;
}

/**
 * Deletes the menu once control returns to the event loop, as it may be the one
 * being used, e.g. when one of its items loads a plugin. Its name is cleared so
 * that it is not found in the meantime, e.g. by tests.
 */
void deleteMenuLater(QMenu* menu)
{
  menu->setObjectName(QString());
  menu->deleteLater();
}
}

//-----------------------------------------------------------------------------
class pqCategoryMenuBarBehavior::pqInternal
{
public:
  QPointer<QMainWindow> MainWindow;
  QPointer<pqProxyGroupMenuManager> MenuManager;
  QList<QPointer<QMenu>> Menus;
};

//-----------------------------------------------------------------------------
pqCategoryMenuBarBehavior::pqCategoryMenuBarBehavior(
  pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow)
  : Superclass(menuManager)
  , Internal(new pqInternal())
{
  assert(menuManager);
  assert(mainWindow);
  this->Internal->MainWindow = mainWindow;
  this->Internal->MenuManager = menuManager;

  QObject::connect(menuManager, &pqProxyGroupMenuManager::menuPopulated, this,
    &pqCategoryMenuBarBehavior::updateMenus);
  QObject::connect(menuManager, &pqProxyGroupMenuManager::categoriesUpdated, this,
    &pqCategoryMenuBarBehavior::updateMenus);
  this->updateMenus();
}

//-----------------------------------------------------------------------------
pqCategoryMenuBarBehavior::~pqCategoryMenuBarBehavior()
{
  // the categories are shown in the menu of the manager again.
  if (this->Internal->MenuManager)
  {
    this->Internal->MenuManager->setShowCategoriesInMenuBar(false);
  }
  for (const auto& menu : this->Internal->Menus)
  {
    if (menu)
    {
      ::deleteMenuLater(menu);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCategoryMenuBarBehavior::updateMenus()
{
  auto& internal = *this->Internal;
  if (!internal.MainWindow || !internal.MenuManager)
  {
    return;
  }

  // use the menu bar of the window, if any: QMainWindow::menuBar() would create
  // one, replacing e.g. a custom menu widget. Without menu bar, the categories
  // are shown in the menu of the manager instead.
  auto menubar = qobject_cast<QMenuBar*>(internal.MainWindow->menuWidget());
  internal.MenuManager->setShowCategoriesInMenuBar(menubar != nullptr);
  internal.Menus.removeAll(nullptr);

  QList<QPointer<QMenu>> menus;
  for (auto category : internal.MenuManager->getMenuCategory()->getCategoriesAlphabetically())
  {
    if (!menubar || !category->showInMenuBar() || category->isEmpty())
    {
      continue;
    }

    QMenu* menu = nullptr;
    for (const auto& existing : internal.Menus)
    {
      if (existing->objectName() == category->name())
      {
        menu = existing;
        break;
      }
    }

    if (menu)
    {
      menu->setTitle(category->label());
      menu->clear();
      for (QMenu* subMenu : menu->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly))
      {
        ::deleteMenuLater(subMenu);
      }
    }
    else
    {
      menu = new QMenu(category->label(), menubar);
      menu->setObjectName(category->name());
      menu->menuAction()->setMenuRole(QAction::NoRole);
      // insert new menus before the Help menu, if any.
      menubar->insertMenu(::findHelpMenuAction(menubar), menu);
    }
    internal.MenuManager->populateMenuFromCategory(menu, category);
    menus << menu;
  }

  // remove the menus of categories no longer shown in the menu bar.
  for (const auto& menu : internal.Menus)
  {
    if (!menus.contains(menu))
    {
      if (auto parentWidget = menu->parentWidget())
      {
        parentWidget->removeAction(menu->menuAction());
      }
      ::deleteMenuLater(menu);
    }
  }
  internal.Menus = menus;
}
