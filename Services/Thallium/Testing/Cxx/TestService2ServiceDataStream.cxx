/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestService2ServiceDataStream.cxx

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
#include <vtkSession.h>

int TestService2ServiceDataStream(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkSession> session;
  session->Initialize("tcp");

  // Setup a service for all the IO.
  auto ioService = session->CreateService("ioService");
  ioService->Start();

  // Setup a service for all the data-processing.
  auto dpService = session->CreateService("dpService");
  dpService->Start();

  auto ioEndpoint = session->CreateServiceEndpoint("ioService");
  if (!ioEndpoint->Connect().Wait())
  {
    vtkLogF(ERROR, "failed to connect to ioService");
    return EXIT_FAILURE;
  }

  auto dpEndpoint = session->CreateServiceEndpoint("dpService");
  if (!dpEndpoint->Connect().Wait())
  {
    vtkLogF(ERROR, "failed to connect to dpService");
    return EXIT_FAILURE;
  }

  // Now, we want dpService to have mechanism to receive data from ioService.
  // This is done using the vtkServiceP2PProvider;
  std::atomic<bool> gotData{ false };
  vtkPacket result;
  auto dsSubscription = dpService->Subscribe("data-stream", "ioService");
  dsSubscription->Wait();
  dsSubscription->GetObservable().subscribe([&result, &gotData](vtkPacket const& packet) {
    vtkLogF(INFO, "dpService got packet");
    result = packet;
    gotData = true;
  });

  ioService->EnableChannelCache("data-stream");
  ioService->GetRequestObservable().subscribe([](vtkServiceReceiver const& receiver) {
    receiver.Publish("data-stream", receiver.GetPacket());
  });

  const vtkPacket data{ { "publish", 12 } };
  ioEndpoint->SendMessage(data);

  vtkLogIfF(ERROR,
    !session->ProcessEventsFor(std::chrono::seconds(1), [&]() -> bool { return gotData; }),
    "failed to receive data on dpService.");

  session->Finalize();
  return result.GetData() == data.GetData() ? EXIT_SUCCESS : EXIT_FAILURE;
}
