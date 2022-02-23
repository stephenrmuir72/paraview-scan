/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumServicesEngineInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThalliumServicesEngineInternals.h"

#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <chrono>
#include <thread>
namespace
{
std::string GetProtocol(const std::string& url)
{
  auto pos = url.find(':');
  return url.substr(0, pos);
}
}

//----------------------------------------------------------------------------
vtkThalliumServicesEngineInternals::vtkThalliumServicesEngineInternals(
  const std::string& url, vtkMultiProcessController* controller)
{
  margo_set_environment(nullptr);
  this->ArgobotsScope.reset(new thallium::abt());

  this->RPCPool =
    thallium::pool::create(thallium::pool::access::mpsc, thallium::pool::kind::fifo_wait);
  this->RPCXstream =
    thallium::xstream::create(thallium::scheduler::predef::basic_wait, *this->RPCPool);

  this->ProgressPool =
    thallium::pool::create(thallium::pool::access::mpsc, thallium::pool::kind::fifo_wait);
  this->ProgressXstream =
    thallium::xstream::create(thallium::scheduler::predef::basic_wait, *this->ProgressPool);

  const auto rank = controller->GetLocalProcessId();
  const auto numRanks = controller->GetNumberOfProcesses();

  if (rank == 0)
  {
    // on root node, create server engine.
    this->Engine.reset(
      new thallium::engine(url, THALLIUM_SERVER_MODE, *this->ProgressPool, *this->RPCPool));
  }
  else
  {
    // on satellites, simply create a client-engine to simply things.
    // we don't really even use the engine on satellites.
    this->Engine.reset(new thallium::engine(
      ::GetProtocol(url), THALLIUM_CLIENT_MODE, *this->ProgressPool, *this->RPCPool));
  }

  this->ProgressPool
    ->make_task([rank, numRanks]() {
      if (numRanks > 1)
      {
        vtkLogger::SetThreadName(("progress." + std::to_string(rank)).c_str());
      }
      else
      {
        vtkLogger::SetThreadName("progress");
      }
    })
    ->join();

  this->Engine->get_handler_pool()
    .make_task([rank, numRanks]() {
      if (numRanks > 1)
      {
        vtkLogger::SetThreadName(("rpc." + std::to_string(rank)).c_str());
      }
      else
      {
        vtkLogger::SetThreadName("rpc");
      }
    })
    ->join();

  // Define handshake.
  this->Engine->define("handshake",
    [this](const thallium::request& req, const std::string& token, const std::string& serviceName) {
      vtkLogScopeF(INFO, "handshake(%s, %s)", token.c_str(), serviceName.c_str());
      // TODO: validate token.

      // let the requester know provider id for requested service, if known.
      auto iter = this->ProviderIds.find(serviceName);
      if (iter != this->ProviderIds.end())
      {
        req.respond(iter->second);
      }
      else
      {
        vtkLogF(ERROR, "Unknown service: %s", serviceName.c_str());
        req.respond(uint16_t(0));
      }
    });
}

//----------------------------------------------------------------------------
vtkThalliumServicesEngineInternals::~vtkThalliumServicesEngineInternals()
{
  try
  {
    // flush the pool
    this->Engine->get_handler_pool()
      .make_task([this]() { vtkLogF(INFO, "finalizing engine"); })
      ->join();
    this->Engine->finalize();
  }
  catch (...)
  {
  }
}

//----------------------------------------------------------------------------
uint16_t vtkThalliumServicesEngineInternals::GetProviderId(const std::string& serviceName)
{
  std::unique_lock<thallium::mutex> lock(this->ProviderIdsMutex);
  auto iter = this->ProviderIds.find(serviceName);
  if (iter != this->ProviderIds.end())
  {
    return iter->second;
  }
  const auto id = this->NextProviderId++;
  this->ProviderIds[serviceName] = id;
  return id;
}
