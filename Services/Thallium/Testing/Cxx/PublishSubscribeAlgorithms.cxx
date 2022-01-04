/*=========================================================================

Program:   Visualization Toolkit
Module:    PublishSubscribeAlgorithms.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkChannelSubscription.h>
#include <vtkDataObject.h>
#include <vtkInformation.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPacket.h>
#include <vtkService.h>
#include <vtkServiceEndpoint.h>
#include <vtkSession.h>
#include <vtkSphereSource.h>
#include <vtkSubscribableAlgorithmOutput.h>
#include <vtkSubscribingAlgorithm.h>

#include <chrono>
#include <thread>

class vtkSubscribingSink : public vtkSubscribingAlgorithm
{
  vtkIdType NumberOfPoints{ 0 };

public:
  static vtkSubscribingSink* New();
  vtkTypeMacro(vtkSubscribingSink, vtkSubscribingAlgorithm);
  vtkSubscribingSink(const vtkSubscribingSink&) = delete;
  void operator=(const vtkSubscribingSink&) = delete;

  vtkGetMacro(NumberOfPoints, vtkIdType);

protected:
  vtkSubscribingSink()
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(0);
  }
  ~vtkSubscribingSink() override = default;

  int RequestData(vtkInformation* /*request*/, vtkInformationVector** inputVector,
    vtkInformationVector* /*outputVector*/) override
  {
    auto* input = vtkDataObject::GetData(inputVector[0], 0);
    this->NumberOfPoints = input->GetNumberOfElements(vtkDataObject::POINT);
    return 1;
  }
};
vtkStandardNewMacro(vtkSubscribingSink);

int PublishSubscribeAlgorithms(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkSession> session;
  session->Initialize("tcp");

  auto producer = session->CreateService("producer");
  auto consumer = session->CreateService("consumer");
  if (!producer->Start() || !consumer->Start())
  {
    vtkLogF(ERROR, "failed to start service(s)");
    session->Finalize();
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkSphereSource> source;
  producer->GetRequestObservable()
    .filter([](const vtkServiceReceiver& receiver) {
      return receiver.GetPacket().GetData().at("command").get_value<std::string>() == "create";
    })
    .subscribe([&source, &producer](const vtkServiceReceiver& receiver) {
      source = vtk::TakeSmartPointer(vtkSphereSource::New());
      receiver.Respond({ { "status", true } });
    });

  producer->GetRequestObservable()
    .filter([](const vtkServiceReceiver& receiver) {
      return receiver.GetPacket().GetData().at("command").get_value<std::string>() == "update";
    })
    .subscribe([&source](const vtkServiceReceiver& receiver) {
      // this will cause the data to be published
      source->Update();
      receiver.Publish("data-stream",
        vtkPacket({ { "data", vtk::MakeSmartPointer(source->GetOutputDataObject(0)) } }));
      receiver.Respond({ { "status", true } });
    });

  vtkNew<vtkSubscribableAlgorithmOutput> port;
  port->SetChannelName("data-stream");
  port->SetServiceName(producer->GetName());

  vtkSmartPointer<vtkSubscribingSink> sink;
  consumer->GetRequestObservable()
    .filter([](const vtkServiceReceiver& receiver) {
      return receiver.GetPacket().GetData().at("command").get_value<std::string>() == "create";
    })
    .subscribe([&sink, &consumer, &port](const vtkServiceReceiver& receiver) {
      sink = vtk::TakeSmartPointer(vtkSubscribingSink::New());
      sink->SetService(consumer);
      sink->SetInputSubscription(port);
      receiver.Respond({ { "status", true } });
    });

  consumer->GetRequestObservable()
    .filter([](const vtkServiceReceiver& receiver) {
      return receiver.GetPacket().GetData().at("command").get_value<std::string>() == "validate";
    })
    .subscribe([&sink](const vtkServiceReceiver& receiver) {
      sink->Update();
      receiver.Respond(
        { { "count", static_cast<int32_t>(sink->GetNumberOfPoints()) }, { "status", true } });
    });

  auto producerEP = session->CreateServiceEndpoint("producer");
  auto consumerEP = session->CreateServiceEndpoint("consumer");
  if (!producerEP->Connect().Wait() || !consumerEP->Connect().Wait())
  {
    vtkLogF(ERROR, "failed to connect to service(s)");
    session->Finalize();
    return EXIT_FAILURE;
  }

  auto success = [](const vtkPacket& packet) {
    try
    {
      return packet.GetData().at("status").get_value<bool>();
    }
    catch (std::out_of_range&)
    {
      return false;
    }
  };

  vtkLogIfF(ERROR, !success(producerEP->SendRequest({ { "command", "create" } }).Wait()),
    "producer 'create' failed");

  vtkLogIfF(ERROR, !success(consumerEP->SendRequest({ { "command", "create" } }).Wait()),
    "consumer 'create' failed");

  vtkLogIfF(ERROR, !success(producerEP->SendRequest({ { "command", "update" } }).Wait()),
    "producer 'update' failed");

  // Wait a bit for messages from producer to pass through to the consumer.
  session->ProcessEventsFor(std::chrono::seconds(1));

  auto result = consumerEP->SendRequest({ { "command", "validate" } });
  vtkLogIfF(ERROR, !success(result.Wait()), "conumer 'validate' failed!");

  const auto value = result.Wait().GetData().at("count").get_value<int32_t>();
  vtkLogIfF(ERROR, value != 50, "Point count mismatch! %d != 50", value);

  vtkLogF(INFO, "shutting down");
  session->Finalize();
  return EXIT_SUCCESS;
}
