/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSerialization.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkDataObject.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPacket.h>
#include <vtkService.h>
#include <vtkServiceEndpoint.h>
#include <vtkSession.h>
#include <vtkSphereSource.h>

#include <chrono>
#include <thread>

template <typename T>
struct DataGenerator
{
  T Last = T();

  T operator()()
  {
    this->Last += static_cast<T>(3.142);
    return this->Last;
  }
};

template <>
struct DataGenerator<bool>
{
  bool Last = true;
  bool operator()()
  {
    this->Last = !this->Last;
    return this->Last;
  }
};

template <>
struct DataGenerator<std::string>
{
  int32_t Last = 0;
  std::string operator()()
  {
    this->Last += 16;
    return std::to_string(this->Last);
  }
};

template <typename T, std::size_t N>
struct DataGenerator<std::array<T, N>>
{
  T Last = 0;
  std::array<T, N> operator()()
  {
    std::array<T, N> array;
    for (auto& item : array)
    {
      item = this->Last;
      this->Last += static_cast<T>(3.142);
    }
    return array;
  }
};

template <typename T>
vtkPacket::VariantMapT CreateMap()
{
  vtkPacket::VariantMapT map;
  DataGenerator<T> generator;
  map["value0"] = generator();
  map["value1"] = generator();

  std::vector<T> values(10);
  std::generate(values.begin(), values.end(), generator);
  map["vector"] = values;
  return map;
};

template <>
vtkPacket::VariantMapT CreateMap<vtkDataObject>()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();
  auto data = vtk::MakeSmartPointer(sphere->GetOutputDataObject(0));
  vtkPacket::VariantMapT map;
  map["value"] = data;
  return map;
}

template <typename T, std::size_t N>
vtkPacket::VariantMapT CreateMap()
{
  vtkPacket::VariantMapT map;
  using ArrayType = std::array<T, N>;
  DataGenerator<ArrayType> generator;
  map["value"] = generator();
  return map;
};

// CREATE_MAP does not accept argument with a comma (,)
// so we need to create some typedefs
using ArrayTypeInt2 = std::array<int, 2>;
using ArrayTypeDouble3 = std::array<double, 3>;
using ArrayTypeDouble6 = std::array<double, 6>;

template <>
vtkPacket::VariantMapT CreateMap<ArrayTypeInt2>()
{
  return CreateMap<int, 2>();
}
template <>
vtkPacket::VariantMapT CreateMap<ArrayTypeDouble3>()
{
  return CreateMap<double, 3>();
}
template <>
vtkPacket::VariantMapT CreateMap<ArrayTypeDouble6>()
{
  return CreateMap<double, 6>();
}

#define CREATE_MAP(_arg)                                                                           \
  if (type == #_arg)                                                                               \
  {                                                                                                \
    auto map = CreateMap<_arg>();                                                                  \
    map["type"] = #_arg;                                                                           \
    return vtkPacket(std::move(map));                                                              \
  }

vtkPacket CreatePacket(const std::string& type)
{
  CREATE_MAP(int32_t);
  CREATE_MAP(bool);
  CREATE_MAP(int64_t);
  CREATE_MAP(uint32_t);
  CREATE_MAP(uint64_t);
  CREATE_MAP(float);
  CREATE_MAP(double);
  CREATE_MAP(std::string);
  // CREATE_MAP(const char*);
  CREATE_MAP(vtkDataObject);
  CREATE_MAP(ArrayTypeInt2);
  CREATE_MAP(ArrayTypeDouble3);
  CREATE_MAP(ArrayTypeDouble6);
  throw std::runtime_error("unknown type " + type);
}

bool Validate(const vtkPacket& packet)
{
  const auto type = packet.GetData().at("type").to_string();
  vtkLogScopeF(INFO, "Validating: %s", type.c_str());
  auto expectedPacket = CreatePacket(type);
  vtkLogF(INFO, "received: %s", packet.ToString().c_str());
  vtkLogF(INFO, "expected: %s", expectedPacket.ToString().c_str());
  return expectedPacket.ToString() == packet.ToString();
}

int TestSerialization(int /*argc*/, char* /*argv*/[])
{
  vtkNew<vtkSession> session;
  session->Initialize("tcp");

  auto service = session->CreateService("service");
  service->GetRequestObservable().subscribe([](const vtkServiceReceiver& receiver) {
    const auto& packet = receiver.GetPacket();
    receiver.Respond({ { "status", Validate(receiver.GetPacket()) } });
  });

  if (!service->Start())
  {
    vtkLogF(ERROR, "Failed to start the service.");
    session->Finalize();
    return EXIT_FAILURE;
  }

  auto endpoint = session->CreateServiceEndpoint("service");
  auto status = endpoint->Connect();
  if (!status.Wait())
  {
    vtkLogF(ERROR, "Failed to connect to service!");
    session->Finalize();
    return EXIT_FAILURE;
  }

  const std::vector<std::string> types{ "bool", "int32_t", "int64_t", "uint32_t", "uint64_t",
    "float", "double", "std::string", "ArrayTypeInt2", "ArrayTypeDouble3", "ArrayTypeDouble6",
    /*"const char*",*/ "vtkDataObject" };
  for (const auto& type : types)
  {
    std::atomic<bool> success{ false };
    endpoint->SendRequest(CreatePacket(type))
      .GetObservable()
      .subscribe([&success](const vtkPacket& packet) {
        success = packet.GetData().at("status").get_value<bool>();
      });
    vtkLogIfF(ERROR,
      !session->ProcessEventsFor(std::chrono::seconds(1), [&]() -> bool { return success; }),
      "serialization failed (or timed out) for '%s'", type.c_str());
  }
  session->Finalize();
  return EXIT_SUCCESS;
}
