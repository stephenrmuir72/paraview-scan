/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGUIApplication.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVGUIApplication
 * @brief
 *
 */

#ifndef vtkPVGUIApplication_h
#define vtkPVGUIApplication_h

#include "pqCoreModule.h" //  for exports
#include "vtkPVApplication.h"

class pqInterfaceTracker;
class pqRecentlyUsedResourcesList;
class pqServerManagerModel;
class pqSettings;
class vtkPVGUIOptions;

class PQCORE_EXPORT vtkPVGUIApplication : public vtkPVApplication
{
public:
  static vtkPVGUIApplication* New();
  vtkTypeMacro(vtkPVGUIApplication, vtkPVApplication);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the singleton once it has been created and initialized.
   */
  static vtkPVGUIApplication* GetInstance();

  /**
   * Provides access to gui options.
   */
  vtkPVGUIOptions* GetOptions() const;

  /**
   * This is a overload that simply uses rxqt::run_loop as the main loop and
   * uses the Qt application to determine executable name.
   */
  bool InitializeUsingQt(vtkMultiProcessController* globalController = nullptr);

  /**
   * Provides access to the pqSettings instance. This is only available after
   * the application has been been successfully initialized.
   */
  pqSettings* GetSettings() const;

  /**
   * Access the interface tracker. Interface tracker holds references to all
   * plugin interface implementations known to the application.
   */
  pqInterfaceTracker* GetInterfaceTracker() const;

  /**
   * Provides access to the server manager model.
   */
  pqServerManagerModel* GetServerManagerModel() const;

  /**
   * List of recently used resources.
   */
  pqRecentlyUsedResourcesList* GetRecentlyUsedResourcesList() const;

protected:
  vtkPVGUIApplication();
  ~vtkPVGUIApplication() override;

  /**
   * Creates a single vtkServerSession. A server application currently only
   * supports a unique server session.
   */
  bool InitializeInternal() override;

  void FinalizeInternal() override;

  /**
   * Overridden to create vtkPVGUIOptions.
   */
  vtkSmartPointer<vtkPVApplicationOptions> CreateOptions() const override;

private:
  vtkPVGUIApplication(const vtkPVGUIApplication&) = delete;
  void operator=(const vtkPVGUIApplication&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
