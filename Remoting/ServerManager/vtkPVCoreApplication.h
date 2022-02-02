/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCoreApplication.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVCoreApplication
 * @brief
 *
 */

#ifndef vtkPVCoreApplication_h
#define vtkPVCoreApplication_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" // for exports
#include "vtkSmartPointer.h"                // for vtkSmartPointer

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

#include <memory> // for std::unique_ptr

class vtkMultiProcessController;
class vtkPVApplicationOptions;
class vtkServicesEngine;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVCoreApplication : public vtkObject
{
public:
  vtkTypeMacro(vtkPVCoreApplication, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the application singleton. This is available once a
   * vtkPVCoreApplication subclass has been created and initialized.
   */
  static vtkPVCoreApplication* GetInstance();

  /**
   * ParaView applications can be of three types.
   */
  enum ApplicationTypes
  {
    UI,
    SERVER,
    BATCH,
  };

  /**
   * Returns the type for this application.
   */
  ApplicationTypes GetApplicationType() const;

  /**
   * Initializes the application.
   *
   * If globalController is nullptr, vtkDummyController will be created and
   * used.
   */
  bool Initialize(const std::string& executable, rxcpp::observe_on_one_worker scheduler,
    vtkMultiProcessController* globalController = nullptr);

  /**
   * Finalizes the application.
   */
  void Finalize();

  /**
   * Provides access to the global controller.
   */
  vtkMultiProcessController* GetController() const;

  /**
   * Returns the number of processes in this process group.
   */
  int GetNumberOfRanks() const;

  /**
   * Returns the local process id.
   */
  int GetRank() const;

  /**
   * Returns the name of the application.
   */
  const std::string& GetApplicationName() const;

  /**
   * Returns the directory that contains the application executable (or empty if
   * unknown).
   *
   * On MacOS, this will point to the actual directory containing the
   * executable, which may be inside an application bundle
   */
  const std::string& GetApplicationDirPath() const;

  /**
   * Returns the file path to the application executable.
   */
  const std::string& GetApplicationFilePath() const;

  /**
   * Returns true if the app has already been initialized.
   */
  bool IsInitialized() const;

  /**
   * Returns an instance of application options. These are generally populated
   * via command line options and affect the application behavior throughout the
   * application lifetime.
   */
  vtkPVApplicationOptions* GetOptions() const;

  /**
   * Provides access to the services engine for this process.
   */
  vtkServicesEngine* GetServicesEngine() const;

  /**
   * Sets the exit code and will cause `WaitForExit` event loop to exit.
   */
  void Exit(int exitCode);

  /**
   * Returns exit code set. If not explicitly set, this will return
   * EXIT_SUCCESS.
   */
  int GetExitCode() const;

  /**
   * Wait for exit. If duration is non-zero, then then timeout after
   * the specified duration of inactivity.
   *
   * Returns false to indicate timeout. Otherwise returns true. The exit code
   * can then be obtained using `GetExitCode()`.
   */
  bool WaitForExit(const rxcpp::schedulers::run_loop& rlp, int ms = 0) const;
  bool WaitForExit(
    const rxcpp::schedulers::run_loop& rlp, const std::chrono::milliseconds& duration) const;

  /**
   * Returns the major version number eg. if version is 2.9.1
   * this method will return 2.
   */
  static int GetVersionMajor();

  /**
   * Returns the minor version number eg. if version is 2.9.1
   * this method will return 9.
   */
  static int GetVersionMinor();

  /**
   * Returns the patch version number eg. if version is 2.9.1
   * this method will return 1.
   */
  static int GetVersionPatch();

  /**
   * Returns a string with the format "paraview version x.x.x, Date: YYYY-MM-DD"
   */
  static const char* GetParaViewSourceVersion();

protected:
  vtkPVCoreApplication(ApplicationTypes type);
  ~vtkPVCoreApplication() override;

  ///@{
  /**
   * Subclasses must override these method to initialize and finalize.
   */
  virtual bool InitializeInternal() = 0;
  virtual void FinalizeInternal() = 0;
  ///@}

  /**
   * Subclasses can override this to customize the URL used to launch
   * the services engine.
   */
  virtual std::string GetEngineURL() const;

  /**
   * Subclasses can override this method to create a vtkPVApplicationOptions
   * subclass, if needed.
   */
  virtual vtkSmartPointer<vtkPVApplicationOptions> CreateOptions() const;

private:
  vtkPVCoreApplication(const vtkPVCoreApplication&) = delete;
  void operator=(const vtkPVCoreApplication&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  static vtkPVCoreApplication* Singleton;
};

#endif
