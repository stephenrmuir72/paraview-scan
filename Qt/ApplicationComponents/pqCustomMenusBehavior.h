// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCustomMenusBehavior_h
#define pqCustomMenusBehavior_h

#include "pqApplicationComponentsModule.h"
#include "pqTimer.h" // for pqTimer

#include <QList>
#include <QObject>
#include <QPointer>

class QAction;
class QMainWindow;
class QMenu;
class QToolBar;
class vtkPVXMLElement;

/**
 * @class pqCustomMenusBehavior
 * @brief adds menus and toolbars described in plugin XML configurations.
 *
 * pqCustomMenusBehavior lets plugins, including XML-only and Python plugins,
 * add menus and toolbars to the main window to expose filters, sources and
 * Python scripts at programmer-defined locations. These are described using a
 * `CustomMenus` element in the plugin's XML, for example:
 *
 * ```xml
 * <ServerManagerConfiguration>
 *   <CustomMenus>
 *     <Menu name="MyTools" label="My Tools" show_in_toolbar="1">
 *       <Proxy group="filters" name="Clip" />
 *       <Proxy group="sources" name="SphereSource" icon=":/pqWidgets/Icons/pqSphere.svg" />
 *       <Separator />
 *       <Script label="Say Hello" tooltip="Prints a greeting.">
 *         print("Hello!")
 *       </Script>
 *       <Script label="Run Analysis" filename="/path/to/analysis.py" />
 *       <Menu label="More Tools">
 *         <Proxy group="filters" name="Contour" />
 *       </Menu>
 *     </Menu>
 *   </CustomMenus>
 * </ServerManagerConfiguration>
 * ```
 *
 * Each top-level `Menu` element adds a menu to the menu bar (unless
 * `show_in_menubar="0"`) and, when `show_in_toolbar="1"`, a toolbar with the
 * same items. `name` identifies the menu and toolbar; when a menu or toolbar with
 * the same name already exists, e.g. when defined by another plugin, items are
 * appended to it. `label` is the text shown to the user, and defaults to `name`.
 *
 * Supported items are:
 * - `Proxy`: creates a filter (`group="filters"`), applied to the selected
 *   pipeline objects, or a source (`group="sources"`). Optional attributes are
 *   `label` and `icon`. By default, the proxy's label and the icon from its
 *   `ShowInMenu` hint, if any, are used.
 * - `Script`: runs Python code, provided either inline or using a `filename`
 *   attribute, then renders all views. Requires Python support. Optional
 *   attributes are `icon` and `tooltip`.
 * - `Separator`: adds a separator.
 * - `Menu`: adds a sub-menu, shown as a drop-down button in toolbars.
 *
 * `icon` can be a Qt resource path (such as `:/pqWidgets/Icons/pqCalculator.svg`)
 * or a file path.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomMenusBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCustomMenusBehavior(QMainWindow* mainWindow);
  ~pqCustomMenusBehavior() override;

protected Q_SLOTS:
  /**
   * Adds menus and toolbars described by the `CustomMenus` elements nested in
   * the given XML, if any.
   */
  void loadConfiguration(vtkPVXMLElement* root);

  /**
   * Updates the enabled state of the actions.
   */
  void updateEnableState();

private:
  Q_DISABLE_COPY(pqCustomMenusBehavior)

  void addTopLevelMenu(vtkPVXMLElement* menuXML);
  QMenu* findOrCreateMenu(const QString& name, const QString& label);
  QToolBar* findOrCreateToolBar(const QString& name, const QString& label);
  void addItems(vtkPVXMLElement* menuXML, QMenu* menu, QToolBar* toolbar);
  QAction* createProxyAction(vtkPVXMLElement* itemXML, QObject* parent);
  QAction* createScriptAction(vtkPVXMLElement* itemXML, QObject* parent);

  QPointer<QMainWindow> MainWindow;
  // used to update the enabled state of the actions after changes are done
  // e.g. after the pipeline objects are applied.
  pqTimer UpdateTimer;
  QList<QPointer<QAction>> FilterActions;
  QList<QPointer<QAction>> SourceActions;
  QList<QPointer<QAction>> ScriptActions;
};

#endif
