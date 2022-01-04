/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestChannels.cxx

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

bool MakeRequestToPublish(const std::string& channel, int messageId, vtkServiceEndpoint* endpoint)
{
  endpoint->SendRequest(vtkPacket({ { "channel", channel }, { "messageId", messageId } })).Wait();
  return true;
}

void TestUncachedChannel(vtkService* /*service*/, vtkServiceEndpoint* endpoint)
{
  vtkLogScopeF(INFO, "TestUncachedChannel");
  // Make service publish messages before subscription is made.
  // They will be lost.
  MakeRequestToPublish("channel-uncached", 100, endpoint);
  MakeRequestToPublish("channel-uncached", 101, endpoint);
  MakeRequestToPublish("channel-uncached", 102, endpoint);

  std::vector<int> receivedIds;
  auto subscription = endpoint->Subscribe("channel-uncached");
  subscription->GetObservable().subscribe([&](const vtkPacket& packet) {
    receivedIds.push_back(packet.GetJSON().at("messageId").get<int>());
  });
  subscription->Wait();

  MakeRequestToPublish("channel-uncached", 201, endpoint);
  MakeRequestToPublish("channel-uncached", 202, endpoint);
  MakeRequestToPublish("channel-uncached", 203, endpoint);

  // Now unsubscribe.
  subscription = nullptr;

  // Now publish a few more messages that should be lost.
  MakeRequestToPublish("channel-uncached", 301, endpoint);
  MakeRequestToPublish("channel-uncached", 302, endpoint);
  MakeRequestToPublish("channel-uncached", 303, endpoint);

  endpoint->GetEngine()->ProcessEventsFor(std::chrono::seconds(1));
  VERIFY(receivedIds == std::vector<int>({ 201, 202, 203 }));
}

void TestCachedChannel(vtkService* service, vtkServiceEndpoint* endpoint)
{
  vtkLogScopeF(INFO, "TestCachedChannel");
  service->EnableChannelCache("channel-cached");

  // Make service publish messages before subscription is made.
  // The last one must *not* get lost.
  MakeRequestToPublish("channel-cached", 100, endpoint);
  MakeRequestToPublish("channel-cached", 101, endpoint);
  MakeRequestToPublish("channel-cached", 102, endpoint);

  std::vector<int> receivedIds;
  auto subscription = endpoint->Subscribe("channel-cached");
  subscription->GetObservable().subscribe([&](const vtkPacket& packet) {
    receivedIds.push_back(packet.GetJSON().at("messageId").get<int>());
  });
  subscription->Wait();

  MakeRequestToPublish("channel-cached", 201, endpoint);
  MakeRequestToPublish("channel-cached", 202, endpoint);
  MakeRequestToPublish("channel-cached", 203, endpoint);

  // Now unsubscribe.
  subscription = nullptr;

  // Now publish a few more messages that should be lost.
  MakeRequestToPublish("channel-cached", 301, endpoint);
  MakeRequestToPublish("channel-cached", 302, endpoint);
  MakeRequestToPublish("channel-cached", 303, endpoint);

  endpoint->GetEngine()->ProcessEventsFor(std::chrono::seconds(1));
  VERIFY(receivedIds == std::vector<int>({ 102, 201, 202, 203 }));

  receivedIds.clear();
  service->ClearChannelCache("channel-cached");

  // Now publish a few more messages.
  MakeRequestToPublish("channel-cached", 401, endpoint);
  MakeRequestToPublish("channel-cached", 402, endpoint);
  MakeRequestToPublish("channel-cached", 403, endpoint);

  // subscribe again.
  subscription = endpoint->Subscribe("channel-cached");
  subscription->GetObservable().subscribe([&](const vtkPacket& packet) {
    receivedIds.push_back(packet.GetJSON().at("messageId").get<int>());
  });
  subscription->Wait();

  service->EnableChannelCache("channel-cached");
  MakeRequestToPublish("channel-cached", 501, endpoint);
  MakeRequestToPublish("channel-cached", 502, endpoint);
  MakeRequestToPublish("channel-cached", 503, endpoint);

  endpoint->GetEngine()->ProcessEventsFor(std::chrono::seconds(1));

  // This time, we only expect the messages sent post subscription.
  VERIFY(receivedIds == std::vector<int>({ 501, 502, 503 }));

  // Create a new subscription to the same channel.
  // Since this is cached channel, we'd expect to get the latest packed.
  // this is a different case since here we already have another subscription
  // active for the same channel.
  receivedIds.clear();
  auto subscription2 = endpoint->Subscribe("channel-cached");
  subscription2->GetObservable().subscribe([&](const vtkPacket& packet) {
    receivedIds.push_back(packet.GetJSON().at("messageId").get<int>());
  });
  subscription->Wait();
  endpoint->GetEngine()->ProcessEventsFor(std::chrono::seconds(1));

  VERIFY(receivedIds == std::vector<int>({ 503 }));
}

int TestChannels(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkServicesEngine> session;
  session->Initialize("tcp");

  auto service = session->CreateService("service1");
  service->GetRequestObservable().subscribe([](const vtkServiceReceiver& receiver) {
    /* every time we receive a request, we publish the same message */
    const auto& packet = receiver.GetPacket();
    const auto channel = packet.GetJSON().at("channel").get<std::string>();
    receiver.Publish(channel, packet);
    receiver.Respond(packet);
  });

  VERIFY(service->Start());

  auto endpoint = session->CreateServiceEndpoint("service1");
  auto status = endpoint->Connect();
  VERIFY(status.Wait());

  TestUncachedChannel(service, endpoint);
  TestCachedChannel(service, endpoint);

  session->Finalize();
  return EXIT_SUCCESS;
}
