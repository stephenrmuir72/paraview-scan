/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCoreApplication.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCoreApplication.h"

#include "vtkCommand.h"
#include "vtkDummyController.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplicationOptions.h"
#include "vtkPVVersion.h" // for PARAVIEW_VERSION_*
#include "vtkReflectionInitializer.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkResourceFileLocator.h"
#include "vtkServicesEngine.h"
#include "vtkSmartPointer.h"

#include <thread>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

#define PARAVIEW_SOURCE_VERSION "paraview version " PARAVIEW_VERSION_FULL

namespace
{
std::string GetSharedResourcesDirectory(const std::string& vtkNotUsed(appDir))
{
  // Look for where the function "GetVTKVersion." lives.
  auto vtk_libs = vtkGetLibraryPathForSymbol(GetVTKVersion);

  // Where docs might be in relation to the executable
  std::vector<std::string> prefixes = {
#if defined(_WIN32) || defined(__APPLE__)
    ".."
#else
    "share/paraview-" PARAVIEW_VERSION
#endif
  };

  // Search for the docs directory
  vtkNew<vtkResourceFileLocator> locator;
  auto resource_dir = locator->Locate(vtk_libs, prefixes, "doc");
  if (!resource_dir.empty())
  {
    resource_dir = vtksys::SystemTools::CollapseFullPath(resource_dir);
  }

  return resource_dir;
}

}

class vtkPVCoreApplication::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkSmartPointer<vtkServicesEngine> Engine;
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkSmartPointer<vtkPVApplicationOptions> Options;
  vtkPVCoreApplication::ApplicationTypes Type;
  std::string ApplicationName;
  std::string ApplicationDirPath;
  std::string ApplicationFilePath;
  std::string SharedResourcesDirectory;
  std::string ExampleFilesDirectory;
  std::string DocDirectory;

  bool Initialized{ false };
  int Rank{ 0 };
  int NumberOfRanks{ 1 };
  int ExitCode{ EXIT_SUCCESS };
  bool ExitCodeSet{ false };

  void SetupApplicationPaths(const std::string argv0)
  {
    // setup app dir/file paths.
    if (argv0.empty())
    {
      this->ApplicationDirPath = vtksys::SystemTools::GetCurrentWorkingDirectory();
      this->ApplicationFilePath = this->ApplicationDirPath + "/unknown_executable";
    }
    else
    {
      std::string errMsg;
      if (!vtksys::SystemTools::FindProgramPath(argv0.c_str(), this->ApplicationFilePath, errMsg))
      {
        // if FindProgramPath fails. We really don't have much of an alternative
        // here. Python module importing is going to fail.
        this->ApplicationFilePath = vtksys::SystemTools::CollapseFullPath(argv0);
      }
      this->ApplicationDirPath = vtksys::SystemTools::GetFilenamePath(this->ApplicationFilePath);
    }

    this->ApplicationName = vtksys::SystemTools::GetFilenameName(this->ApplicationFilePath);
    this->ApplicationName =
      vtksys::SystemTools::GetFilenameWithoutLastExtension(this->ApplicationName);

    this->SharedResourcesDirectory = ::GetSharedResourcesDirectory(this->ApplicationDirPath);
    this->ExampleFilesDirectory = this->SharedResourcesDirectory = "/examples";
    this->DocDirectory = this->SharedResourcesDirectory = "/doc";
  }

  std::string GetRankAnnotatedName(const std::string& name) const
  {
    return name.empty() || this->NumberOfRanks <= 1 ? name
                                                    : name + "." + std::to_string(this->Rank);
  }

  std::string GetCSURL() const
  {
    const std::string url = this->Engine->GetUrl();
    vtksys::RegularExpression regEx("^[^:]+://(.*)$");
    if (!regEx.find(url))
    {
      vtkLogF(INFO, "URL in unrecognized form ('%s'). Using default.", url.c_str());
      abort();
    }

    return fmt::format("cs://{}", regEx.match(1));
  }
};

vtkPVCoreApplication* vtkPVCoreApplication::Singleton;
//----------------------------------------------------------------------------
vtkPVCoreApplication::vtkPVCoreApplication(vtkPVCoreApplication::ApplicationTypes type)
  : Internals(new vtkPVCoreApplication::vtkInternals())
{
  auto& internals = (*this->Internals);
  internals.Type = type;
}

//----------------------------------------------------------------------------
vtkPVCoreApplication::~vtkPVCoreApplication()
{
  vtkLogIfF(ERROR, vtkPVCoreApplication::Singleton == this,
    "vtkPVCoreApplication singleton is non-null. Did you forget to call 'Finalize'?");
}

//----------------------------------------------------------------------------
vtkPVCoreApplication* vtkPVCoreApplication::GetInstance()
{
  return vtkPVCoreApplication::Singleton;
}

//----------------------------------------------------------------------------
vtkPVCoreApplication::ApplicationTypes vtkPVCoreApplication::GetApplicationType() const
{
  const auto& internals = (*this->Internals);
  return internals.Type;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVCoreApplication::GetController() const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return internals.Controller;
}

//----------------------------------------------------------------------------
int vtkPVCoreApplication::GetNumberOfRanks() const
{
  const auto& internals = (*this->Internals);
  return internals.NumberOfRanks;
}

//----------------------------------------------------------------------------
int vtkPVCoreApplication::GetRank() const
{
  const auto& internals = (*this->Internals);
  return internals.Rank;
}

//----------------------------------------------------------------------------
const std::string& vtkPVCoreApplication::GetApplicationName() const
{
  const auto& internals = (*this->Internals);
  return internals.ApplicationName;
}

//----------------------------------------------------------------------------
const std::string& vtkPVCoreApplication::GetApplicationDirPath() const
{
  const auto& internals = (*this->Internals);
  return internals.ApplicationDirPath;
}

//----------------------------------------------------------------------------
const std::string& vtkPVCoreApplication::GetApplicationFilePath() const
{
  const auto& internals = (*this->Internals);
  return internals.ApplicationFilePath;
}

//----------------------------------------------------------------------------
bool vtkPVCoreApplication::IsInitialized() const
{
  const auto& internals = (*this->Internals);
  return internals.Initialized;
}

//----------------------------------------------------------------------------
bool vtkPVCoreApplication::Initialize(const std::string& executable,
  rxcpp::observe_on_one_worker scheduler, vtkMultiProcessController* globalController)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  vtkLogger::SetThreadName(internals.ApplicationName.c_str());

  static bool reflection_initialized = false;
  if (!reflection_initialized)
  {
    vtkLogF(TRACE, "Initialize VTK-object reflection");
    auto* reflection = vtkReflectionInitializer::New();
    if (!reflection)
    {
      vtkLogF(ERROR, "Reflection has not been initialized correctly!");
      abort();
    }
    reflection->Initialize();
    reflection->Delete();
    reflection_initialized = true;
  }

  if (vtkPVCoreApplication::Singleton)
  {
    throw std::runtime_error("Cannot initialize multiple ParaView-based applications.");
  }

  internals.SetupApplicationPaths(executable);
  auto* options = this->GetOptions(); // ensure options object is setup.
  options->SetReadOnly(true);

  if (globalController)
  {
    internals.Controller = globalController;
  }
  else
  {
    internals.Controller = vtk::TakeSmartPointer(vtkDummyController::New());
    internals.Controller->Initialize(nullptr, nullptr);
  }

  internals.Rank = internals.Controller->GetLocalProcessId();
  internals.NumberOfRanks = internals.Controller->GetNumberOfProcesses();
  vtkLogger::SetThreadName(internals.GetRankAnnotatedName(internals.ApplicationName));

  if (options->GetLogStdErrVerbosity() != vtkLogger::VERBOSITY_INVALID)
  {
    vtkLogger::SetStderrVerbosity(options->GetLogStdErrVerbosity());
  }

  for (const auto& log_pair : options->GetLogFiles())
  {
    vtkLogger::LogToFile(
      internals.GetRankAnnotatedName(log_pair.first).c_str(), vtkLogger::TRUNCATE, log_pair.second);
  }

  // TODO: vtkProcessModule::InitializePythonEnvironment()

  // Startup services engine.
  internals.Engine = vtk::TakeSmartPointer(vtkServicesEngine::New());
  // TODO: should we pass a duplicated controller here?
  internals.Engine->SetController(internals.Controller);
  internals.Engine->Initialize(this->GetEngineURL(), scheduler);
  vtkPVCoreApplication::Singleton = this;

  if (!this->InitializeInternal())
  {
    vtkPVCoreApplication::Singleton = nullptr;
    return false;
  }

  vtkLogF(TRACE, "application initialized");
  internals.Initialized = true;
  if (internals.Rank == 0 && internals.Type == vtkPVCoreApplication::SERVER &&
    !internals.ExitCodeSet)
  {
    cout << "Accepting connection: " << internals.GetCSURL() << endl;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPVCoreApplication::Finalize()
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (vtkPVCoreApplication::Singleton == this)
  {
    this->InvokeEvent(vtkCommand::ExitEvent);
    this->FinalizeInternal();
    vtkPVCoreApplication::Singleton = nullptr;
  }
}

//----------------------------------------------------------------------------
vtkServicesEngine* vtkPVCoreApplication::GetServicesEngine() const
{
  const auto& internals = (*this->Internals);
  return internals.Engine;
}

//----------------------------------------------------------------------------
vtkPVApplicationOptions* vtkPVCoreApplication::GetOptions() const
{
  auto& internals = (*this->Internals);
  if (internals.Options == nullptr)
  {
    vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
    internals.Options = this->CreateOptions();
  }
  return internals.Options;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPVApplicationOptions> vtkPVCoreApplication::CreateOptions() const
{
  return vtk::TakeSmartPointer(vtkPVApplicationOptions::New());
}

//----------------------------------------------------------------------------
std::string vtkPVCoreApplication::GetEngineURL() const
{
  return "sockets://";
}

//-----------------------------------------------------------------------------
void vtkPVCoreApplication::Exit(int exitCode)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.ExitCodeSet = true;
  internals.ExitCode = exitCode;
}

//-----------------------------------------------------------------------------
int vtkPVCoreApplication::GetExitCode() const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return internals.ExitCodeSet ? internals.ExitCode : EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
bool vtkPVCoreApplication::WaitForExit(const rxcpp::schedulers::run_loop& rlp, int ms) const
{
  return this->WaitForExit(rlp, std::chrono::milliseconds(ms));
}

//-----------------------------------------------------------------------------
bool vtkPVCoreApplication::WaitForExit(
  const rxcpp::schedulers::run_loop& rlp, const std::chrono::milliseconds& duration) const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  auto* controller = this->GetController();
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

//----------------------------------------------------------------------------
const char* vtkPVCoreApplication::GetParaViewSourceVersion()
{
  return PARAVIEW_SOURCE_VERSION;
}

//----------------------------------------------------------------------------
int vtkPVCoreApplication::GetVersionMajor()
{
  return PARAVIEW_VERSION_MAJOR;
}

//----------------------------------------------------------------------------
int vtkPVCoreApplication::GetVersionMinor()
{
  return PARAVIEW_VERSION_MINOR;
}

//----------------------------------------------------------------------------
int vtkPVCoreApplication::GetVersionPatch()
{
  return PARAVIEW_VERSION_PATCH;
}

//----------------------------------------------------------------------------
const std::string& vtkPVCoreApplication::GetSharedResourcesDirectory() const
{
  const auto& internals = (*this->Internals);
  return internals.SharedResourcesDirectory;
}

//----------------------------------------------------------------------------
const std::string& vtkPVCoreApplication::GetExampleFilesDirectory() const
{
  const auto& internals = (*this->Internals);
  return internals.ExampleFilesDirectory;
}

//----------------------------------------------------------------------------
const std::string& vtkPVCoreApplication::GetDocDirectory() const
{
  const auto& internals = (*this->Internals);
  return internals.DocDirectory;
}

//----------------------------------------------------------------------------
void vtkPVCoreApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
