/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEndpointUnsubscribe.cxx

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

/**
 * Connect an endpoint to a service through a channel and after a
 * number of messages unsubscribe from the channel.
 */
int TestEndpointUnsubscribe(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkServicesEngine> session;
  session->Initialize("tcp");

  auto service = session->CreateService("service1");
  service->GetRequestObservable().subscribe([](const vtkServiceReceiver& receiver) {
    vtkLogScopeF(INFO, "handle message or request: %s", receiver.GetPacket().ToString().c_str());
    receiver.Respond(vtkPacket{});
  });

  VERIFY(service->Start());

  auto endpoint = session->CreateServiceEndpoint("service1");
  auto status = endpoint->Connect();
  VERIFY(status.Wait());

  std::atomic<int> packets_received{ 0 };
  vtkSmartPointer<vtkChannelSubscription> subscription = endpoint->Subscribe("channel0");
  subscription->Wait();
  subscription->GetObservable().subscribe([&packets_received](const vtkPacket& /* packet */) {
    vtkLogF(INFO, "got packet on channel0");
    ++packets_received;
  });

  std::atomic<int> packets_sent{ 0 };
  // publish packets on channel0 adding small delay
  rxcpp::observable<>::interval(std::chrono::milliseconds(100))
    .take(6)
    .observe_on(service->GetRunLoopScheduler())
    .subscribe([&service, &packets_sent](int idx) {
      vtkLogF(INFO, "sending packet #%d", idx);
      ++packets_sent;
      service->Publish("channel0", vtkNJson({ "idx", idx }));
    });

  vtkLogIfF(ERROR,
    !session->ProcessEventsFor(std::chrono::seconds(20),
      [&]() { return packets_sent == packets_received && packets_sent == 6; }),
    "packets not sent/received correctly!");

  // now terminate the subscription.
  subscription = nullptr;

  // This is necessary to ensure that the unsubscription message get sent.
  session->ProcessEventsFor(std::chrono::milliseconds(200));

  rxcpp::observable<>::interval(std::chrono::milliseconds(100))
    .take(4)
    .observe_on(service->GetRunLoopScheduler())
    .subscribe([&service, &packets_sent](int idx) {
      idx += 6;
      vtkLogF(INFO, "sending packet #%d", idx);
      ++packets_sent;
      service->Publish("channel0", vtkNJson({ "idx", idx }));
    });

  // wait till more messages are sent.
  vtkLogIfF(ERROR,
    !session->ProcessEventsFor(std::chrono::seconds(20), [&]() { return packets_sent == 10; }),
    "packets not sent correctly!");

  vtkLogF(INFO, "shutting down");
  session->Finalize();

  vtkLogF(INFO, "packets sent %d - received %d", static_cast<int>(packets_sent),
    static_cast<int>(packets_received));

  if (packets_sent == 0)
  {
    vtkLogF(ERROR, "No packets sent by the service");
    return EXIT_FAILURE;
  }
  if (packets_received == 0)
  {
    vtkLogF(ERROR, "No packets received by the endpoint");
    return EXIT_FAILURE;
  }
  if (packets_sent == packets_received)
  {
    vtkLogF(ERROR, "No packets skipped! Unsubscribe may have failed !");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
