/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide PARAVIEW_DEPRECATED_IN_5_10_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "vtkProcessModule.h"

#include "vtkCLIOptions.h"
#include "vtkCommand.h"
#include "vtkCompositeDataSet.h"
#include "vtkDummyController.h"
#include "vtkFloatingPointExceptions.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiThreader.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkPSystemTools.h"
#include "vtkPolyData.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkServicesEngine.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPIController.h"
#endif

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonInterpreter.h"
#endif

#ifdef _WIN32
#include "vtkDynamicLoader.h"
#else
#include <csignal>
#endif

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

/// FIXME: ASYNC
/// // this include is needed to ensure that vtkPVPluginLoader singleton doesn't get
/// // destroyed before the process module singleton is cleaned up.
/// #include "vtkPVPluginLoader.h"

#include <cassert>
#include <clocale> // needed for setlocale()
#include <sstream>
#include <stdexcept> // for runtime_error

namespace
{

static std::string GetCSUrl(const std::string& url)
{
  vtksys::RegularExpression regEx("^[^:]+://(.*)$");
  if (!regEx.find(url))
  {
    vtkLogF(INFO, "URL in unrecognized form ('%s'). Using default.", url.c_str());
    abort();
  }

  return fmt::format("cs://{}", regEx.match(1));
}

// This is used to avoid creating vtkWin32OutputWindow on ParaView executables.
// vtkWin32OutputWindow is not a useful window for any of the ParaView commandline
// executables.
class vtkPVGenericOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkPVGenericOutputWindow, vtkOutputWindow);
  static vtkPVGenericOutputWindow* New();

private:
  vtkPVGenericOutputWindow() = default;
  ~vtkPVGenericOutputWindow() override = default;
};
vtkStandardNewMacro(vtkPVGenericOutputWindow);

static void UpdateThreadName(
  vtkProcessModule::ProcessTypes type, vtkMultiProcessController* controller)
{
  // set thread name based on application.
  std::string tname_suffix;
  if (controller->GetNumberOfProcesses() > 1)
  {
    tname_suffix = "." + std::to_string(controller->GetLocalProcessId());
  }
  switch (type)
  {
    case vtkProcessModule::PROCESS_CLIENT:
      vtkLogger::SetThreadName("paraview" + tname_suffix);
      break;
    case vtkProcessModule::PROCESS_SERVER:
      vtkLogger::SetThreadName("pvserver" + tname_suffix);
      break;
    case vtkProcessModule::PROCESS_BATCH:
      vtkLogger::SetThreadName("pvbatch" + tname_suffix);
      break;
    default:
      break;
  }
}

void HandleDisplay(int& argc, char**& argv)
{
  // This is largely left for legacy purposes. There are better ways of specifying
  // display options.

  // Hack to support -display parameter.  CLI Options requires parameters to be
  // specified as -option=value, but it is generally expected that X window
  // programs allow you to set the display as -display host:port (i.e. without
  // the = between the option and value).
  for (int i = 1; i < argc - 1; i++)
  {
    if (strcmp(argv[i], "-display") == 0)
    {
      size_t size = strlen(argv[i + 1]) + 10;
      char* displayenv = new char[size];
      snprintf(displayenv, size, "DISPLAY=%s", argv[i + 1]);
      vtksys::SystemTools::PutEnv(displayenv);
      delete[] displayenv;
      // safe to delete since PutEnv keeps a copy of the string.
      argc -= 2;
      for (int j = i; j < argc; j++)
      {
        argv[j] = argv[j + 2];
      }
      argv[argc] = nullptr;
      break;
    }
  }
}
}

//============================================================================
class vtkProcessModule::vtkInternals
{
  vtkSmartPointer<vtkMultiProcessController> GlobalController;

public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkSmartPointer<vtkServicesEngine> Engine;
  std::string ProgramPath;
  std::string SelfDir;
  int Rank{ 0 };
  int NumberOfRanks{ 1 };
  std::atomic<bool> ReportInterpreterErrors{ true };
  int ExitCode{ EXIT_SUCCESS };
  bool ExitCodeSet{ false };

  vtkInternals() = default;
  ~vtkInternals()
  {
    if (this->Engine)
    {
      this->Engine->Finalize();
    }

    if (this->GlobalController)
    {
      this->GlobalController->Finalize(/*finalizedExternally*/ 1);
    }
  }

  void SetGlobalController(vtkMultiProcessController* controller)
  {
    this->GlobalController = controller;
    this->Rank = controller->GetLocalProcessId();
    this->NumberOfRanks = controller->GetNumberOfProcesses();
  }

  vtkMultiProcessController* GetGlobalController() const { return this->GlobalController; }
};

//============================================================================
// * STATICS
vtkProcessModule::ProcessTypes vtkProcessModule::ProcessType = vtkProcessModule::PROCESS_INVALID;

bool vtkProcessModule::FinalizeMPI = false;
bool vtkProcessModule::FinalizePython = false;

vtkSmartPointer<vtkProcessModule> vtkProcessModule::Singleton;

int vtkProcessModule::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines = 1;
int vtkProcessModule::DefaultMinimumGhostLevelsToRequestForStructuredPipelines = 0;

//----------------------------------------------------------------------------
bool vtkProcessModule::Initialize(
  ProcessTypes type, int& argc, char**& argv, rxcpp::observe_on_one_worker scheduler)
{
  setlocale(LC_NUMERIC, "C");

  vtkProcessModule::ProcessType = type;
  vtkSmartPointer<vtkMultiProcessController> globalController =
    vtkSmartPointer<vtkDummyController>::New();

  // This needs special handling. Generally, `vtkProcessModule::Initialize` is
  // called first and then all arguments are processed. This is required since
  // the arguments may be modified by the `MPI_Init` call and hence we should
  // let MPI_Init process the args before the application processes them. The
  // only exception is args that affect `vtkProcessModule::Initialize` itself.
  // Hence, we handle those here explicitly.
  auto config = vtkProcessModuleConfiguration::GetInstance();
  vtkNew<vtkCLIOptions> options;
  options->GenerateWarningsOff();
  config->PopulateOptions(options, type);

  // At this point, the args are loaded with stuff we don't parse,
  // so it's essential to set allow-extras to true.
  options->SetAllowExtras(true);
  options->Parse(argc, argv);

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  // scan the arguments to determine if we need to initialize MPI on client.
  bool use_mpi;
  if (type == PROCESS_CLIENT)
  {
#if defined(PARAVIEW_INITIALIZE_MPI_ON_CLIENT)
    use_mpi = true;
#else
    use_mpi = false;
#endif
  }
  else
  {
    use_mpi = true;
  }

  if (config->GetForceMPIInit())
  {
    use_mpi = true;
  }
  else if (config->GetForceNoMPIInit())
  {
    use_mpi = false;
  }

  // initialize MPI only on all processes if paraview is compiled w/MPI.
  int mpi_already_initialized = 0;
  MPI_Initialized(&mpi_already_initialized);
  if (mpi_already_initialized == 0 && use_mpi)
  {
    // MPICH changes the current working directory after MPI_Init. We fix that
    // by changing the CWD back to the original one after MPI_Init.
    std::string cwd = vtksys::SystemTools::GetCurrentWorkingDirectory();

    int provided_threading;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided_threading);

    // restore CWD to what it was before the MPI initialization.
    vtksys::SystemTools::ChangeDirectory(cwd);

    vtkProcessModule::FinalizeMPI = true;
  } // END if MPI is already initialized

  if (use_mpi || mpi_already_initialized)
  {
    int provided_threading;
    MPI_Query_thread(&provided_threading);
    if (provided_threading != MPI_THREAD_MULTIPLE)
    {
      throw std::runtime_error("MPI_THREAD_MULTIPLE not supported!");
    }

    if (vtkMPIController* controller =
          vtkMPIController::SafeDownCast(vtkMultiProcessController::GetGlobalController()))
    {
      globalController = controller;
    }
    else
    {
      globalController = vtkSmartPointer<vtkMPIController>::New();
      globalController->Initialize(&argc, &argv, /*initializedExternally*/ 1);
    }
    // Ensure that the user cannot run a client with more than one rank.
    if (type == PROCESS_CLIENT && globalController->GetNumberOfProcesses() > 1)
    {
      throw std::runtime_error("Client process should be run with one process!");
    }
  }
#else
  static_cast<void>(argc); // unused warning when MPI is off
  static_cast<void>(argv); // unused warning when MPI is off
#endif
  globalController->BroadcastTriggerRMIOn();
  vtkMultiProcessController::SetGlobalController(globalController);

  const int rank = globalController->GetLocalProcessId();

  // Setup logging
  UpdateThreadName(type, globalController);
  if (config->GetLogStdErrVerbosity() != vtkLogger::VERBOSITY_INVALID)
  {
    vtkLogger::SetStderrVerbosity(
      static_cast<vtkLogger::Verbosity>(config->GetLogStdErrVerbosity()));
  }
  vtkLogger::Init(argc, argv, nullptr);
  for (const auto& log_pair : config->GetLogFiles())
  {
    vtkLogger::LogToFile(
      vtkProcessModuleConfiguration::GetRankAnnotatedFileName(log_pair.first).c_str(),
      vtkLogger::TRUNCATE, log_pair.second);
  }

  // This is left over for legacy cases, just in case users are still using
  // this.
  HandleDisplay(argc, argv);

#ifdef _WIN32
  // Avoid Ghost windows on windows XP
  typedef void (*VOID_FUN)();
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary("user32.dll");
  if (lib)
  {
    VOID_FUN func =
      (VOID_FUN)vtkDynamicLoader::GetSymbolAddress(lib, "DisableProcessWindowsGhosting");
    if (func)
    {
      (*func)();
    }
  }
#endif // _WIN32

#ifdef PARAVIEW_ENABLE_FPE
  vtkFloatingPointExceptions::Enable();
#endif

  if (vtkProcessModule::ProcessType != PROCESS_CLIENT)
  {
    // On non-client processes, we don't want VTK default output window esp. on
    // Windows since that pops up too many windows. Hence we replace it.
    vtkNew<vtkPVGenericOutputWindow> window;
    vtkOutputWindow::SetInstance(window.GetPointer());
  }

  // In general turn off error prompts. This is where the process waits for
  // user-input on any error/warning. In past, we turned on prompts on Windows.
  // However, for client processes, we capture the error messages in the UI and
  // for the server-processes, prompts can cause unnecessary interruptions hence
  // we turn off prompts all together.
  vtkOutputWindow::GetInstance()->PromptUserOff();

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  if (config->GetUseMPISSend())
  {
    vtkMPIController::SetUseSsendForRMI(1);
  }
#endif

  vtkMultiThreader::SetGlobalMaximumNumberOfThreads(1);

  // The running dashboard tests avoid showing the abort/retry popup dialog.
  vtksys::SystemTools::EnableMSVCDebugHook();

#ifndef _WIN32
  // When trying to send data to a socket/pipe and that socket/pipe is
  // closed/broken, the system will trigger a SIGPIPE signal which has by
  // default a quit/crash handler that won't give you any chance to handle the
  // error in any way.
  // Instead, we ignore the default handler to properly capture the error and
  // eventually provide some type of recovery.
  signal(SIGPIPE, SIG_IGN);
#endif

  // Create the process module.
  vtkProcessModule::Singleton = vtk::TakeSmartPointer(vtkProcessModule::New());
  vtkProcessModule::Singleton->Internals->SetGlobalController(globalController);
  vtkProcessModule::Singleton->DetermineExecutablePath(argc, argv);
  vtkProcessModule::Singleton->InitializePythonEnvironment();

  // Setup services engine.
  auto engine = vtk::TakeSmartPointer(vtkServicesEngine::New());
  // TODO: should we pass a duplicated controller here?
  engine->SetController(globalController);
  engine->Initialize("tcp://", scheduler);
  vtkProcessModule::Singleton->Internals->Engine = engine;

  // unset global controller; we don't want VTK filters to use this controller,
  // instead it should use service specific controller.
  vtkMultiProcessController::SetGlobalController(nullptr);
  if (rank == 0 && type == vtkProcessModule::PROCESS_SERVER)
  {
    cout << "Accepting connection(s): " << ::GetCSUrl(engine->GetUrl()) << endl;
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkProcessModule::Finalize()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  // Finalize Python before anything else. This ensures that all proxy
  // references are removed before the process module disappears.
  if (vtkProcessModule::FinalizePython && vtkPythonInterpreter::IsInitialized())
  {
    vtkPythonInterpreter::Finalize();
  }
#endif

  if (vtkProcessModule::Singleton)
  {
    vtkProcessModule::Singleton->InvokeEvent(vtkCommand::ExitEvent);
  }

  // destroy the process-module.
  vtkProcessModule::Singleton = nullptr;

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  if (vtkProcessModule::FinalizeMPI)
  {
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    // prevent caling MPI_Finalize() twice
    vtkProcessModule::FinalizeMPI = false;
  }
#endif

  return true;
}

//----------------------------------------------------------------------------
vtkProcessModule::ProcessTypes vtkProcessModule::GetProcessType()
{
  return vtkProcessModule::ProcessType;
}

//----------------------------------------------------------------------------
vtkProcessModule* vtkProcessModule::GetProcessModule()
{
  return vtkProcessModule::Singleton.GetPointer();
}

//----------------------------------------------------------------------------
bool vtkProcessModule::GetSymmetricMPIMode()
{
  return vtkProcessModuleConfiguration::GetInstance()->GetSymmetricMPIMode();
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetDefaultMinimumGhostLevelsToRequestForStructuredPipelines(int val)
{
  vtkProcessModule::DefaultMinimumGhostLevelsToRequestForStructuredPipelines = val;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines()
{
  return vtkProcessModule::DefaultMinimumGhostLevelsToRequestForStructuredPipelines;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines(int val)
{
  vtkProcessModule::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines = val;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines()
{
  return vtkProcessModule::DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetNumberOfGhostLevelsToRequest(vtkInformation* info)
{
  auto* pm = vtkProcessModule::GetProcessModule();
  if (pm == nullptr || pm->GetNumberOfRanks() <= 1)
  {
    return 0;
  }

  vtkDataSet* ds = vtkDataSet::GetData(info);
  if (ds || vtkCompositeDataSet::GetData(info) != nullptr)
  {
    // Check if this is structured-pipeline, this includes unstructured pipelines
    // downstream from structured source e.g. Wavelet - > Clip.
    // To do that, we use a trick. If WHOLE_EXTENT() key us present, it must have
    // started as a structured dataset.
    // The is one exception to this for ExplicitStructuredGrid data, their
    // behavior is both structured and unstructured but regarding ghost cells
    // they have the behavior of an unstructured dataset.
    const bool is_structured = (info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) != 0) &&
      (!ds || (ds && !ds->IsA("vtkExplicitStructuredGrid")));
    return is_structured
      ? vtkProcessModule::GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines()
      : vtkProcessModule::GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines();
  }
  else
  {
    // the pipeline is for a non-dataset/composite-dataset filter. don't bother
    // asking for ghost levels.
    return 0;
  }
}

//============================================================================
// * vtkProcessModule non-static methods
vtkStandardNewMacro(vtkProcessModule);
//----------------------------------------------------------------------------
vtkProcessModule::vtkProcessModule()
  : Internals(new vtkProcessModule::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkProcessModule::~vtkProcessModule() = default;

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkProcessModule::GetGlobalController() const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return internals.GetGlobalController();
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetNumberOfRanks() const
{
  const auto& internals = (*this->Internals);
  return internals.NumberOfRanks;
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetRank() const
{
  const auto& internals = (*this->Internals);
  return internals.Rank;
}

//----------------------------------------------------------------------------
bool vtkProcessModule::IsMPIInitialized() const
{
  const auto& internals = (*this->Internals);
  return (internals.GetGlobalController()->IsA("vtkMPIController") != 0);
}

//----------------------------------------------------------------------------
void vtkProcessModule::DetermineExecutablePath(int argc, char* argv[])
{
  assert(argc >= 1);

  auto& internals = (*this->Internals);
  if (argc > 0)
  {
    std::string errMsg;
    if (!vtkPSystemTools::FindProgramPath(argv[0], internals.ProgramPath, errMsg))
    {
      // if FindProgramPath fails. We really don't have much of an alternative
      // here. Python module importing is going to fail.
      internals.ProgramPath = vtkPSystemTools::CollapseFullPath(argv[0]);
    }
    internals.SelfDir = vtksys::SystemTools::GetFilenamePath(internals.ProgramPath);
  }
  else
  {
    internals.SelfDir = vtkPSystemTools::GetCurrentWorkingDirectory(/*collapse=*/true);
    internals.ProgramPath = internals.SelfDir + "/unknown_exe";
  }
}

//----------------------------------------------------------------------------
bool vtkProcessModule::InitializePythonEnvironment()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
  if (!vtkPythonInterpreter::IsInitialized())
  {
    // If someone already initialized Python before ProcessModule was started,
    // we don't finalize it when ProcessModule finalizes. This is for the cases
    // where ParaView modules are directly imported in python (not pvpython).
    vtkProcessModule::FinalizePython = true;
  }

// Help with finding `vtk` (and `paraview`) packages. This is generally needed
// only for static builds, but no harm in doing for shared build as well.
#if BUILD_SHARED_LIBS
  vtkPythonInterpreter::PrependPythonPath(this->GetSelfDir().c_str(), "vtkmodules/__init__.py");
#else
  // for static builds, we use zipped packages.

  // add path to _vtk.zip which is also the location for mpi4py modules, if any.
  vtkPythonInterpreter::PrependPythonPath(
    this->GetSelfDir().c_str(), "_vtk.zip", /*add_landmark=*/false);

  // add path for _vtk.zip; all vtk modules will be found here.
  vtkPythonInterpreter::PrependPythonPath(
    this->GetSelfDir().c_str(), "_vtk.zip", /*add_landmark=*/true);

  // add path for _paraview.zip, these are all the ParaView modules.
  vtkPythonInterpreter::PrependPythonPath(
    this->GetSelfDir().c_str(), "_paraview.zip", /*add_landmark=*/true);
#endif

#if defined(_WIN32)
  // ParaView executables generally link with all modules built except a few
  // such as the Catalyst libraries e.g. vtkPVCatalyst.dll. Now, when importing its
  // Python module `vtkPVCatalystPython.pyd` for example, it will need to load
  // vtkPVCatalyst.dll from its standard load paths which do not include the
  // executable path where the vtkPVCatalyst.dll actually exists. Hence we
  // manually extend the PATH to include it. This is not an issue on unixes
  // since rpath (or LD_LIBRARY_PATH set by shared forwarding executables) takes
  // care of it.
  std::ostringstream stream;
  stream << "PATH=" << this->SelfDir;
  if (const char* oldpath = vtksys::SystemTools::GetEnv("PATH"))
  {
    stream << ";" << oldpath;
  }
  vtksys::SystemTools::PutEnv(stream.str());
#endif // defined(_WIN32)

#endif // VTK_MODULE_ENABLE_VTK_PythonInterpreter
  return true;
}

//----------------------------------------------------------------------------
bool vtkProcessModule::GetReportInterpreterErrors() const
{
  const auto& internals = (*this->Internals);
  return internals.ReportInterpreterErrors;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetReportInterpreterErrors(bool state)
{
  auto& internals = (*this->Internals);
  internals.ReportInterpreterErrors = state;
}

//----------------------------------------------------------------------------
const std::string& vtkProcessModule::GetProgramPath() const
{
  const auto& internals = (*this->Internals);
  return internals.ProgramPath;
}

//----------------------------------------------------------------------------
const std::string& vtkProcessModule::GetSelfDir() const
{
  const auto& internals = (*this->Internals);
  return internals.SelfDir;
}

//----------------------------------------------------------------------------
void vtkProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkServicesEngine* vtkProcessModule::GetServicesEngine() const
{
  const auto& internals = (*this->Internals);
  return internals.Engine;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiProcessController> vtkProcessModule::NewController() const
{
  auto* worldController = this->GetGlobalController();
  if (vtkDummyController::SafeDownCast(worldController))
  {
    auto controller = vtk::TakeSmartPointer(vtkDummyController::New());
    controller->Initialize(nullptr, nullptr);
    return controller;
  }

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  auto* worldComm = vtkMPICommunicator::SafeDownCast(worldController->GetCommunicator());
  auto* mpiHandle = worldComm->GetMPIComm()->GetHandle();

  MPI_Comm clone;
  MPI_Comm_dup(*mpiHandle, &clone);

  vtkMPICommunicatorOpaqueComm opaqueComm(&clone);
  vtkNew<vtkMPICommunicator> cloneComm;
  cloneComm->InitializeExternal(&opaqueComm);

  vtkNew<vtkMPIController> controller;
  controller->SetCommunicator(cloneComm);
  // ensure we use broadcast/collectives for RMI rather than p2p messaging.
  controller->BroadcastTriggerRMIOn();

  return controller.GetPointer();
#else
  auto controller = vtk::TakeSmartPointer(vtkDummyController::New());
  controller->Initialize(nullptr, nullptr);
  return controller;
#endif
}

//-----------------------------------------------------------------------------
void vtkProcessModule::Exit(int exitCode)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.ExitCodeSet = true;
  internals.ExitCode = exitCode;
}

//-----------------------------------------------------------------------------
int vtkProcessModule::GetExitCode() const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return internals.ExitCodeSet ? internals.ExitCode : EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
bool vtkProcessModule::WaitForExit(const rxcpp::schedulers::run_loop& rlp, int ms) const
{
  return this->WaitForExit(rlp, std::chrono::milliseconds(ms));
}

//-----------------------------------------------------------------------------
bool vtkProcessModule::WaitForExit(
  const rxcpp::schedulers::run_loop& rlp, const std::chrono::milliseconds& duration) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  auto* controller = this->GetGlobalController();
  if (internals.Rank > 0)
  {
    controller->ProcessRMIs();
  }
  else
  {
    auto start = std::chrono::system_clock::now();
    while (!internals.ExitCodeSet)
    {
      while (!rlp.empty() && rlp.peek().when < rlp.now())
      {
        rlp.dispatch();
        start = std::chrono::system_clock::now();
      }
      if (duration.count() != 0 && (std::chrono::system_clock::now() - start) >= duration)
      {
        // timed out!
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (internals.NumberOfRanks > 1)
    {
      controller->TriggerBreakRMIs();
    }
  }
  controller->Barrier();
  return internals.ExitCodeSet;
}
