/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBasicRemoting.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkChannelSubscription.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPacket.h>
#include <vtkService.h>
#include <vtkServiceEndpoint.h>
#include <vtkServicesEngine.h>

#include <chrono>
#include <thread>

#define VERIFY(x)                                                                                  \
  if (!(x))                                                                                        \
  {                                                                                                \
    vtkLogF(ERROR, "Failed " #x);                                                                  \
    std::terminate();                                                                              \
  }

int TestBasicRemoting(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkServicesEngine> session;
  session->Initialize("tcp");

  auto service = session->CreateService("service1");
  VERIFY(service != nullptr);

  // setup service handler. this can be done before the service is started.
  service->GetRequestObservable()
    .filter([](const vtkServiceReceiver& receiver) {
      try
      {
        return receiver.GetPacket().GetJSON().at("type").get<std::string>() == "echo";
      }
      catch (std::out_of_range&)
      {
        return true;
      }
    })
    .subscribe([](const vtkServiceReceiver& receiver) {
      vtkLogScopeF(INFO, "echoing: %s", receiver.GetPacket().ToString().c_str());
      receiver.Respond(receiver.GetPacket());
    });

  VERIFY(service->Start() == true);

  auto endpoint = session->CreateServiceEndpoint("service1");
  VERIFY(endpoint != nullptr);

  auto status = endpoint->Connect();
  VERIFY(status.Wait());

  const vtkPacket msgPS{ { "value", 199 } };
  std::atomic<int> count{ 0 };
  auto channelSubscription = endpoint->Subscribe("channel0");
  channelSubscription->GetObservable().subscribe([&](const vtkPacket& packet) {
    vtkLogF(INFO, "got packet #%d on channel0", (count + 1));
    count += (packet == msgPS) ? 1 : 0;
  });
  channelSubscription->Wait();

  endpoint->SendMessage(vtkPacket({ { "int32", 1021 }, { "c-string", "hello-world" },
    { "std-string", std::string("hello world!") }, { "float", 102.4F }, { "double", 10.24 },
    { "bool", false }, { "type", "echo" } }));

  vtkLogF(INFO, "send request");
  std::atomic<bool> success{ false };
  vtkPacket msg0({ { "type", "echo" }, { "tag", 1 } });
  auto reply = endpoint->SendRequest(msg0).Wait();
  vtkLogF(INFO, "send: %s\ngot: %s", msg0.ToString().c_str(), reply.ToString().c_str());
  vtkLogIfF(ERROR, reply != msg0, "tag:1 reply mismatched!");

  success = false;
  vtkPacket msg1({ { "type", "echo" }, { "tag", 2 } });
  success = endpoint->SendRequest(msg1).Wait() == msg1;
  vtkLogIfF(ERROR, !success, "tag:2 reply mismatched!");

  vtkLogF(INFO, "test-pub/sub");
  auto values = rxcpp::observable<>::from(msgPS, msgPS);
  values.observe_on(rxcpp::observe_on_run_loop(service->GetRunLoop()))
    .as_blocking()
    .subscribe([&service](const vtkPacket& packet) { service->Publish("channel0", packet); });

  vtkLogIfF(ERROR,
    !session->ProcessEventsFor(std::chrono::seconds(2), [&]() { return count == 2; }),
    "Failed pub/sub");

  channelSubscription = nullptr;
  session->Finalize();
  return EXIT_SUCCESS;
}
