/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkServicesEngine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkServicesEngine.h"

#include "vtkDummyController.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkService.h"
#include "vtkServiceEndpoint.h"
#include "vtkSmartPointer.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPIController.h"
#endif

#include <map>
#include <mutex>

class vtkServicesEngine::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  std::shared_ptr<rxcpp::schedulers::run_loop> RunLoop;
  std::unique_ptr<rxcpp::observe_on_one_worker> Coordination;

  vtkSmartPointer<vtkMultiProcessController> Controller{ vtk::TakeSmartPointer(
    vtkDummyController::New()) };

  std::mutex ServicesMutex;
  std::map<std::string, vtkSmartPointer<vtkService>> Services;

  std::mutex EndpointsMutex;
  std::map<std::string, vtkSmartPointer<vtkServiceEndpoint>> Endpoints;

  std::string Url;
  std::atomic<bool> Initialized{ false };
  std::atomic<bool> Finalized{ false };
};

vtkAbstractObjectFactoryNewMacro(vtkServicesEngine);
//----------------------------------------------------------------------------
vtkServicesEngine::vtkServicesEngine()
  : Internals(new vtkServicesEngine::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkServicesEngine::~vtkServicesEngine()
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  for (const auto& pair : internals.Services)
  {
    pair.second->UnsetEngine();
  }
}

//----------------------------------------------------------------------------
void vtkServicesEngine::SetController(vtkMultiProcessController* controller)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (internals.Initialized)
  {
    vtkLogF(ERROR, "Controller cannot be changed after initialization!");
    return;
  }
  internals.Controller = controller;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkServicesEngine::GetController() const
{
  const auto& internals = (*this->Internals);
  return internals.Controller;
}

//----------------------------------------------------------------------------
const std::string& vtkServicesEngine::GetUrl() const
{
  const auto& internals = (*this->Internals);
  if (!internals.Initialized)
  {
    vtkLogF(ERROR, "Session has not been initialized!");
  }

  if (internals.Finalized)
  {
    vtkLogF(ERROR, "Session has been finalized!");
  }

  return internals.Url;
}

//----------------------------------------------------------------------------
void vtkServicesEngine::Initialize(const std::string& url)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  this->Initialize(url, nullptr);
}

//----------------------------------------------------------------------------
void vtkServicesEngine::Initialize(
  const std::string& url, const std::shared_ptr<rxcpp::schedulers::run_loop>& runLoop)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.RunLoop = runLoop ? runLoop : std::make_shared<rxcpp::schedulers::run_loop>();
  this->Initialize(url, rxcpp::observe_on_run_loop(*internals.RunLoop));
}

//----------------------------------------------------------------------------
void vtkServicesEngine::Initialize(const std::string& url, rxcpp::observe_on_one_worker cordination)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (internals.Initialized)
  {
    vtkLogF(ERROR, "Cannot reinitialized an already initialized session!");
    std::terminate();
  }

  if (internals.Finalized)
  {
    vtkLogF(ERROR, "Cannot reuse an already finalized session!");
    std::terminate();
  }

  vtkLogF(INFO, "%s: initializing session (%s)", vtkLogIdentifier(this), url.c_str());
  internals.Coordination.reset(new rxcpp::observe_on_one_worker(std::move(cordination)));
  internals.Url = this->InitializeInternal(url);
  internals.Initialized = true;
}

//----------------------------------------------------------------------------
void vtkServicesEngine::Finalize()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (internals.Initialized && !internals.Finalized)
  {
    vtkLogScopeF(INFO, "%s: finalizing session", vtkLogIdentifier(this));
    internals.Finalized = true;
    internals.Initialized = false;

    { // scope for lock
      const std::lock_guard<std::mutex> lockEP(internals.EndpointsMutex);
      for (auto const& pair : internals.Endpoints)
      {
        if (pair.second)
        {
          pair.second->Shutdown();
        }
      }
    }

    { // scope for lock
      const std::lock_guard<std::mutex> lockEP(internals.ServicesMutex);
      for (auto const& s : internals.Services)
      {
        if (s.second)
        {
          s.second->Shutdown();
        }
      }
    }

    this->FinalizeInternal();
    internals.Url.clear();
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkService> vtkServicesEngine::CreateService(const std::string& serviceName)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (internals.Finalized)
  {
    vtkLogF(ERROR, "Session has been finalized!");
    return nullptr;
  }

  if (!internals.Initialized)
  {
    vtkLogF(ERROR, "Session has not been initialized. Cannot create service.");
    return nullptr;
  }

  const std::lock_guard<std::mutex> lock(internals.ServicesMutex);
  if (internals.Services.find(serviceName) != internals.Services.end())
  {
    vtkLogF(ERROR, "Duplicate service name: %s", serviceName.c_str());
    return nullptr;
  }

  auto service = vtk::TakeSmartPointer(vtkService::New());
  if (service == nullptr)
  {
    vtkLogF(ERROR,
      "Failed to create a specific `vtkService`. "
      "Is the VTK object-factory setup correctly?");
    return nullptr;
  }

  internals.Services[serviceName] = service;
  service->Initialize(this, serviceName);
  return service;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkService> vtkServicesEngine::GetService(const std::string& serviceName) const
{
  auto& internals = (*this->Internals);
  if (internals.Finalized)
  {
    vtkLogF(ERROR, "Session has been finalized!");
    return nullptr;
  }

  if (!internals.Initialized)
  {
    vtkLogF(ERROR, "Session has not been initialized. Cannot create service.");
    return nullptr;
  }

  const std::lock_guard<std::mutex> lock(internals.ServicesMutex);
  auto iter = internals.Services.find(serviceName);
  if (iter == internals.Services.end())
  {
    vtkLogF(ERROR, "Unknown service '%s'", serviceName.c_str());
    return nullptr;
  }

  return iter->second;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkServiceEndpoint> vtkServicesEngine::CreateServiceEndpoint(
  const std::string& serviceName, const std::string& urlArg)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (internals.Finalized)
  {
    vtkLogF(ERROR, "Session has been finalized!");
    return nullptr;
  }

  if (!internals.Initialized)
  {
    vtkLogF(ERROR, "Session has not been initialized. Cannot create service-endpoint");
    return nullptr;
  }

  const auto url = urlArg.empty() ? this->GetUrl() : urlArg;
  const std::unique_lock<std::mutex> lock(internals.EndpointsMutex);
  auto iter = internals.Endpoints.find(serviceName);
  if (iter != internals.Endpoints.end())
  {
    vtkLogF(ERROR, "endpoint already exists!");
    return nullptr;
  }

  auto endpoint = vtk::TakeSmartPointer(vtkServiceEndpoint::New());
  if (endpoint == nullptr)
  {
    vtkLogF(ERROR,
      "Failed to create a specific `vtkServiceEndpoint`. "
      "Is the VTK object-factory setup correctly?");
    return nullptr;
  }

  endpoint->Initialize(this, serviceName, url);
  internals.Endpoints[serviceName] = endpoint.GetPointer();
  return endpoint;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkServiceEndpoint> vtkServicesEngine::GetServiceEndpoint(
  const std::string& serviceName) const
{
  auto& internals = (*this->Internals);
  if (internals.Finalized)
  {
    vtkLogF(ERROR, "Session has been finalized!");
    return nullptr;
  }

  if (!internals.Initialized)
  {
    vtkLogF(ERROR, "Session has not been initialized. Cannot create service-endpoint");
    return nullptr;
  }

  const std::unique_lock<std::mutex> lock(internals.EndpointsMutex);
  auto iter = internals.Endpoints.find(serviceName);
  if (iter == internals.Endpoints.end())
  {
    vtkLogF(ERROR, "Unknown endpoint for '%s'", serviceName.c_str());
    return nullptr;
  }

  return iter->second;
}

//----------------------------------------------------------------------------
rxcpp::observe_on_one_worker vtkServicesEngine::GetCoordination() const
{
  auto& internals = (*this->Internals);
  return (*internals.Coordination);
}

//----------------------------------------------------------------------------
bool vtkServicesEngine::ProcessEvents() const
{
  const auto& internals = (*this->Internals);
  if (!internals.RunLoop)
  {
    vtkLogF(WARNING,
      "ProcessEvents can only be used when `rxcpp::schedulers::run_loop` "
      "is being used.");
    return false;
  }

  bool status = false;
  const auto& rlp = (*internals.RunLoop);
  while ((!rlp.empty() && rlp.peek().when < rlp.now()))
  {
    rlp.dispatch();
    status = true;
  }

  return status;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMultiProcessController> vtkServicesEngine::NewController() const
{
  auto* worldController = this->GetController();
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

//----------------------------------------------------------------------------
void vtkServicesEngine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
