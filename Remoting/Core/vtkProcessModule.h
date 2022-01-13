/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkProcessModule
 * @brief process initialization and management core for ParaView processes.
 *
 * vtkProcessModule initializes a ParaView process. Overtime, the
 * responsibilities of this class have reduced considerably. Now it simply
 * initializes the MPI and Python environments, as applicable based on the
 * process type.
 */
#ifndef vtkProcessModule_h
#define vtkProcessModule_h

#include "vtkObject.h"
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_10_0
#include "vtkRemotingCoreModule.h"  //needed for exports
#include "vtkSmartPointer.h"        // needed for vtkSmartPointer.

#include <memory> // for std::unique_ptr
#include <string> // for std::string

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkInformation;
class vtkMultiProcessController;
class vtkServicesEngine;

class VTKREMOTINGCORE_EXPORT vtkProcessModule : public vtkObject
{
public:
  vtkTypeMacro(vtkProcessModule, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ProcessTypes
  {
    PROCESS_CLIENT, /* Capable of connecting to remote server or self.
                      Cannot run in Parallel */
    PROCESS_SERVER, /* data-server+render-server */
    PROCESS_BATCH,  /* Capable of running in parallel with root node acting as client.
               Cannot connect to remote server */
    PROCESS_INVALID = -1,
  };

  static ProcessTypes GetProcessType();
  static unsigned int GetProcessTypeAsInt()
  {
    return static_cast<unsigned int>(vtkProcessModule::GetProcessType());
  }

  /**
   * Initializes the process. Will create the vtkProcessModule singleton..
   */
  static bool Initialize(
    ProcessTypes type, int& argc, char**& argv, rxcpp::observe_on_one_worker scheduler);

  /**
   * Finalizes and cleans up the process.
   */
  static bool Finalize();

  /**
   * Provides access to the global ProcessModule. This method can only be called
   * after Initialize().
   */
  static vtkProcessModule* GetProcessModule();

  /**
   * Provides access to the global MPI controller, if any. If not running in a
   * distributed environment, this may return a vtkDummyController.
   */
  vtkMultiProcessController* GetGlobalController() const;

  /**
   * Returns the number of processes in this process group.
   */
  int GetNumberOfRanks() const;

  /**
   * Returns the local process id.
   */
  int GetRank() const;

  /**
   * Return whether MPI is initialized in this process group.
   */
  bool IsMPIInitialized() const;

  //@{
  /**
   * Set/Get whether to report errors from the Interpreter.
   */
  bool GetReportInterpreterErrors() const;
  void SetReportInterpreterErrors(bool state);
  vtkBooleanMacro(ReportInterpreterErrors, bool);
  //@}

  //@{
  static bool GetSymmetricMPIMode();
  //@}

  /**
   * The full path to the current executable that is running (or empty if unknown).
   */
  const std::string& GetProgramPath() const;

  /**
   * The directory containing the current executable (or empty if unknown).
   */
  const std::string& GetSelfDir() const;

  //@{
  /**
   * This is temporary approach to control the number of ghost-levels to request
   * by default for data pipelines. Currently, change in ghost level
   * request causes the pipeline to re-execute which can be expensive. In an
   * ideal world, additional ghost levels can be automatically provided by
   * data-exchange between ranks. Until we do that, this is only mechanism
   * available to override the number of ghost levels requested by default.
   *
   * The default is 0 for structured pipelines, and 1 for unstructured
   * pipelines. When not running in parallel, however, these ghost-level
   * requests don't make sense and hence are generally ignored.
   *
   * Note, this is expected to change in the future, so use this with
   * caution.
   */
  static void SetDefaultMinimumGhostLevelsToRequestForStructuredPipelines(int);
  static int GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines();
  static void SetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines(int);
  static int GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines();
  //@}

  /**
   * This returns number of ghost level to request based on characteristics of
   * the pipelines.
   */
  static int GetNumberOfGhostLevelsToRequest(vtkInformation* outInfo);

  /**
   * Provides access to the services engine for this process.
   */
  vtkServicesEngine* GetServicesEngine() const;

  /**
   * This is used to create a new controller that is a duplicate of the main
   * controller.
   *
   * When using MPI, this will be a controller with a different handle.
   */
  vtkSmartPointer<vtkMultiProcessController> NewController() const;

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

protected:
  vtkProcessModule();
  ~vtkProcessModule() override;

private:
  static vtkProcessModule* New();
  vtkProcessModule(const vtkProcessModule&) = delete;
  void operator=(const vtkProcessModule&) = delete;

  void DetermineExecutablePath(int argc, char** argv);

  // Helper to initialize Python environment. This doesn't initialize Python
  // but simply sets up the environment so when Python is initialized, it can
  // find ParaView modules. This does nothing if not build with Python support.
  bool InitializePythonEnvironment();

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  static ProcessTypes ProcessType;

  // Set to true in Initialize if Finalize() should cleanup MPI.
  static bool FinalizeMPI;

  // Set to true in InitializePythonEnvironment() if Finalize() should cleanup
  // Python.
  static bool FinalizePython;
  static vtkSmartPointer<vtkProcessModule> Singleton;
  static int DefaultMinimumGhostLevelsToRequestForStructuredPipelines;
  static int DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines;
};

#endif // vtkProcessModule_h
