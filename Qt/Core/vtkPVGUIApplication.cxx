/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGUIApplication.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGUIApplication.h"

#include "pqInterfaceTracker.h"
#include "pqObjectBuilder.h"
#include "pqProgressManager.h"
#include "pqRecentlyUsedResourcesList.h"
#include "pqServerConfigurationCollection.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSettings.h"
#include "pqStandardServerManagerModelInterface.h"
#include "vtkObjectFactory.h"
#include "vtkPVGUIOptions.h"
#include "vtkPVLogger.h"
#include "vtkSMViewProxy.h"

#include <QApplication>
#include <QScopedPointer>

#include <vtk_rxqt.h>
// clang-format off
#include VTK_REMOTING_RXQT(rxqt.hpp)
// clang-format on

namespace
{

// creates a new pqSettings object.
pqSettings* CreateSettings(vtkPVGUIApplication* self)
{
  auto* options = self->GetOptions();
  const auto disableRegistry = options->GetDisableRegistry();
  const auto orgName = QApplication::organizationName();
  const auto appName = QApplication::applicationName() + QApplication::applicationVersion() +
    (disableRegistry ? "_bogus" : "");
  auto* settings = new pqSettings(QSettings::IniFormat, QSettings::UserScope, orgName, appName);
  if (disableRegistry || settings->value("vtkPVGUIApplication.DisableSettings", false).toBool())
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "loading of Qt settings skipped (disabled).");
    settings->clear();
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "loading Qt settings from '%s'",
      qPrintable(settings->fileName()));
  }
  return settings;
}

} // namespace {}

//----------------------------------------------------------------------------
class vtkPVGUIApplication::vtkInternals
{
public:
  std::unique_ptr<rxqt::run_loop> RxQtRunLoop;
  QScopedPointer<pqSettings> Settings;
  QScopedPointer<pqInterfaceTracker> InterfaceTracker;
  QScopedPointer<pqServerManagerObserver> ServerManagerObserver;
  QScopedPointer<pqServerManagerModel> ServerManagerModel;
  QScopedPointer<pqRecentlyUsedResourcesList> RecentlyUsedResourcesList;
  QScopedPointer<pqProgressManager> ProgressManager;
  QScopedPointer<pqObjectBuilder> ObjectBuilder;
  QScopedPointer<pqServerConfigurationCollection> ServerConfigurations;
};

vtkStandardNewMacro(vtkPVGUIApplication);
//----------------------------------------------------------------------------
vtkPVGUIApplication::vtkPVGUIApplication()
  : Internals(new vtkPVGUIApplication::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVGUIApplication::~vtkPVGUIApplication() = default;

//----------------------------------------------------------------------------
vtkPVGUIApplication* vtkPVGUIApplication::GetInstance()
{
  return vtkPVGUIApplication::SafeDownCast(vtkPVCoreApplication::GetInstance());
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPVApplicationOptions> vtkPVGUIApplication::CreateOptions() const
{
  return vtk::TakeSmartPointer(vtkPVGUIOptions::New());
}

//----------------------------------------------------------------------------
vtkPVGUIOptions* vtkPVGUIApplication::GetOptions() const
{
  return vtkPVGUIOptions::SafeDownCast(this->Superclass::GetOptions());
}

//----------------------------------------------------------------------------
bool vtkPVGUIApplication::InitializeUsingQt(vtkMultiProcessController* globalController)
{
  auto& internals = (*this->Internals);
  internals.RxQtRunLoop.reset(new rxqt::run_loop());
  return this->Superclass::Initialize(QCoreApplication::applicationFilePath().toLocal8Bit().data(),
    internals.RxQtRunLoop->observe_on_run_loop(), globalController);
}

//----------------------------------------------------------------------------
bool vtkPVGUIApplication::InitializeInternal()
{
  if (!this->Superclass::InitializeInternal())
  {
    return false;
  }

  auto& internals = (*this->Internals);

  // Need to ensure the process creates generic render windows
  vtkSMViewProxy::SetUseGenericOpenGLRenderWindow(true);

  internals.Settings.reset(::CreateSettings(this));
  internals.InterfaceTracker.reset(new pqInterfaceTracker());
  internals.InterfaceTracker->addInterface(
    new pqStandardServerManagerModelInterface(internals.InterfaceTracker.get()));
  internals.RecentlyUsedResourcesList.reset(new pqRecentlyUsedResourcesList());
  internals.RecentlyUsedResourcesList->load(*internals.Settings.get());

  internals.ServerManagerObserver.reset(new pqServerManagerObserver());
  internals.ServerManagerModel.reset(
    new pqServerManagerModel(internals.ServerManagerObserver.get()));
  internals.ObjectBuilder.reset(new pqObjectBuilder());

  internals.ProgressManager.reset(new pqProgressManager());

  internals.ServerConfigurations.reset(new pqServerConfigurationCollection());
  return true;
}

//----------------------------------------------------------------------------
void vtkPVGUIApplication::FinalizeInternal()
{
  auto& internals = (*this->Internals);
  internals.ServerConfigurations.reset();
  this->Superclass::FinalizeInternal();
}

//----------------------------------------------------------------------------
pqSettings* vtkPVGUIApplication::GetSettings() const
{
  const auto& internals = (*this->Internals);
  return internals.Settings.get();
}

//----------------------------------------------------------------------------
pqInterfaceTracker* vtkPVGUIApplication::GetInterfaceTracker() const
{
  const auto& internals = (*this->Internals);
  return internals.InterfaceTracker.get();
}

//----------------------------------------------------------------------------
pqServerManagerModel* vtkPVGUIApplication::GetServerManagerModel() const
{
  const auto& internals = (*this->Internals);
  return internals.ServerManagerModel.get();
}

//----------------------------------------------------------------------------
pqServerManagerObserver* vtkPVGUIApplication::GetServerManagerObserver() const
{
  const auto& internals = (*this->Internals);
  return internals.ServerManagerObserver.get();
}

//----------------------------------------------------------------------------
pqRecentlyUsedResourcesList* vtkPVGUIApplication::GetRecentlyUsedResourcesList() const
{
  const auto& internals = (*this->Internals);
  return internals.RecentlyUsedResourcesList.get();
}

//----------------------------------------------------------------------------
pqProgressManager* vtkPVGUIApplication::GetProgressManager() const
{
  const auto& internals = (*this->Internals);
  return internals.ProgressManager.get();
}

//----------------------------------------------------------------------------
pqObjectBuilder* vtkPVGUIApplication::GetObjectBuilder() const
{
  const auto& internals = (*this->Internals);
  return internals.ObjectBuilder.get();
}

//----------------------------------------------------------------------------
pqServerConfigurationCollection* vtkPVGUIApplication::GetServerConfigurations() const
{
  const auto& internals = (*this->Internals);
  return internals.ServerConfigurations.get();
}

//----------------------------------------------------------------------------
void vtkPVGUIApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
