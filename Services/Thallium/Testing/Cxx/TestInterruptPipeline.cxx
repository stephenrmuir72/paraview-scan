/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestInterruptPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkJson.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPacket.h>
#include <vtkService.h>
#include <vtkServiceEndpoint.h>
#include <vtkServicesEngine.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

#include <map>

namespace
{

vtkSmartPointer<vtkAlgorithm> Create(const std::string& type)
{
  if (type == "vtkSphereSource")
  {
    return vtk::TakeSmartPointer(vtkSphereSource::New());
  }

  return nullptr;
}

};

int TestInterruptPipeline(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkServicesEngine> session;
  session->Initialize("tcp");

  std::map<int32_t, vtkSmartPointer<vtkAlgorithm>> algorithms;

  auto service = session->CreateService("service1");
  service->GetRequestObservable(/*skipEventLoop*/ true)
    .subscribe([&algorithms, &service](const vtkServiceReceiver& receiver) {
      const auto& packet = receiver.GetPacket();
      const auto& json = packet.GetJSON();
      const auto command = json.at("command").get<std::string>();
      if (command == "create")
      {
        auto id = json.at("id").get<int32_t>();
        auto type = json.at("type").get<std::string>();
        vtkLogScopeF(INFO, "Got request to create (id=%d, type=%s)", id, type.c_str());

        // create object on the service's main thread.
        auto worker = service->GetRunLoopScheduler().create_coordinator().get_worker();
        worker.schedule([&algorithms, type, id](const rxcpp::schedulers::schedulable&) {
          vtkLogF(INFO, "creating (id=%d, type=%s)", id, type.c_str());
          // TODO: here we could create a full pipeline, instead of just 1 source at
          // a time to make it easier to test that use-case.
          auto algorithm = ::Create(type);
          algorithms[id] = algorithm;
        });

        receiver.Respond(vtkJson{ { "status", true } });
      }
      else if (command == "update")
      {
        auto id = json.at("id").get<int32_t>();
        vtkLogScopeF(INFO, "Got request to update (id=%d)", id);

        // update on the service's main thread.
        auto worker = service->GetRunLoopScheduler().create_coordinator().get_worker();
        worker.schedule([&algorithms, id](const rxcpp::schedulers::schedulable&) {
          vtkLogF(INFO, "Updating (id=%d)", id);
          algorithms[id]->Update();
        });
        receiver.Respond(vtkJson{ { "status", true } });
      }
      else if (command == "modify_n_update")
      {
        auto id = json.at("id").get<int32_t>();
        vtkLogScopeF(INFO, "Got request to modify_n_update (id=%d)", id);

        // TODO: determine how to do this, we want to interrupt any existing
        // update happening and then retrigger an update.
        receiver.Respond(vtkJson{ { "status", true } });
      }
    });

  service->Start();
  auto endpoint = session->CreateServiceEndpoint("service1");
  auto status = endpoint->Connect();
  status.Wait();

  endpoint
    ->SendRequest(vtkJson({ { "id", 1 }, { "command", std::string{ "create" } },
      { "type", std::string{ "vtkSphereSource" } } }))
    .Wait();

  endpoint->SendRequest(vtkJson({ { "id", 1 }, { "command", std::string{ "update" } } })).Wait();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  endpoint->SendRequest(vtkJson({ { "id", 1 }, { "command", std::string{ "modify_n_update" } } }))
    .Wait();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  session->Finalize();
  return EXIT_SUCCESS;
}
