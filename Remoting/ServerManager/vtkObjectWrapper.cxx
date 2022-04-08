/*=========================================================================

  Program:   ParaView
  Module:    vtkObjectWrapper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectWrapper.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkObjectStore.h"
#include "vtkProxyDefinitionManager.h"
#include "vtkReflection.h"
#include "vtkRemoteObjectProvider.h"
#include "vtkSMProperty.h" // for vtkSMProperty::PropertyTypes
#include "vtkService.h"
#include "vtkSmartPointer.h"
#include "vtkStringUtilities.h"

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

#include <algorithm>

// lightweight property information
struct PropertyInformation
{
  std::string Method;
  int Type{ vtkSMProperty::INVALID };
  bool InformationOnly{ false };
};

class vtkObjectWrapper::vtkInternals
{
public:
  vtkTypeUInt32 GlobalID{ 0 };
  std::string VTKClassName;
  std::string LogName;
  int Rank{ 0 };
  int NumberOfRanks{ 1 };
  vtkSmartPointer<vtkObject> VTKObject;
  std::map<std::string, PropertyInformation> Properties;
  std::map<std::string, std::string> Commands;
  std::map<std::string, std::string> SubproxyCommands;
  std::string PostPushCommand;
};

vtkStandardNewMacro(vtkObjectWrapper);
//----------------------------------------------------------------------------
vtkObjectWrapper::vtkObjectWrapper()
  : Internals(new vtkObjectWrapper::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkObjectWrapper::~vtkObjectWrapper() = default;

//----------------------------------------------------------------------------
bool vtkObjectWrapper::ReadXMLAttributes(const pugi::xml_node& node)
{
  auto& internals = (*this->Internals);
  internals.VTKClassName = node.attribute("class").as_string();
  internals.PostPushCommand = node.attribute("post_push").as_string();

  // process properties.
  for (auto child : node.children())
  {
    if (vtkStringUtilities::EndsWith(child.name(), "Property"))
    {
      PropertyInformation info;
      info.Type = vtkSMProperty::GetPropertyType(fmt::format("vtkSM{}", child.name()));
      if (info.Type == vtkSMProperty::INVALID)
      {
        continue;
      }
      info.Method = child.attribute("command").as_string();
      info.InformationOnly = child.attribute("information_only").as_bool(false);
      internals.Properties.emplace(child.attribute("name").as_string(), std::move(info));
    }
    else if (strcmp(child.name(), "Command") == 0)
    {
      auto name = child.attribute("name").as_string();
      internals.Commands.emplace(name, child.attribute("method").as_string(name));
    }
    else if (strcmp(child.name(), "SubProxy") == 0)
    {
      auto* name = child.attribute("name").as_string();
      if (auto* command = child.attribute("command").as_string(nullptr))
      {
        internals.SubproxyCommands.emplace(name, command);
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkObjectWrapper::Initialize(
  vtkTypeUInt32 gid, const vtkNJson& state, vtkRemoteObjectProvider* provider)
{
  auto& internals = (*this->Internals);
  internals.GlobalID = gid;
  if (!internals.VTKClassName.empty())
  {
    internals.VTKObject = vtkReflection::CreateInstance(internals.VTKClassName);
    if (!internals.VTKObject)
    {
      vtkLogF(ERROR, "Failed to create class '%s'", internals.VTKClassName.c_str());
      return false;
    }

    // Pass gloabl-id to  VTK object;
    vtkReflection::Set(
      internals.VTKObject, "SetGlobalID", std::vector<int>{ static_cast<int>(gid) });

    // Pass controller to the VTK object; this is a no-op of the "SetController"
    // is not supported by the VTKObject. Passing the controller is crucial to
    // ensure that the VTK object uses the service-specific distributed controller,
    // if it needs one.
    auto* controller = provider->GetController();
    vtkReflection::Set(internals.VTKObject, "SetController", std::vector<vtkObject*>{ controller });
    internals.Rank = controller->GetLocalProcessId();
    internals.NumberOfRanks = controller->GetNumberOfProcesses();

    auto* service = provider->GetService();
    if (service->GetName() == "ds")
    {
      vtkReflection::Set(
        internals.VTKObject, "InitializeForDataProcessing", std::vector<vtkVariant>{});
    }
    else
    {
      vtkReflection::Set(internals.VTKObject, "InitializeForRendering", std::vector<vtkVariant>{});
    }

    // Handle subproxy "command"s.
    auto objectStore = provider->GetObjectStore();
    for (const auto& el : state.at("subproxies").items())
    {
      const auto& name = el.key();
      const auto id = el.value().get<vtkTypeUInt32>();
      auto iter = internals.SubproxyCommands.find(name);
      if (id != 0 && iter != internals.SubproxyCommands.end())
      {
        vtkReflection::Set(internals.VTKObject, iter->second,
          std::vector<vtkObject*>{ objectStore->FindVTKObject<vtkObject>(id).GetPointer() });
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkObjectWrapper::UpdateState(const vtkNJson& state, vtkRemoteObjectProvider* provider)
{
  auto& internals = (*this->Internals);
  internals.LogName = state.at("log_name").get<std::string>();

  // Clean up the abort flag of the algorithm
  // FIXME multiple update states will mess up the flag . Use message tags and reset abort only for the last one
  if (vtkAlgorithm* algorithm = vtkAlgorithm::SafeDownCast(this->GetVTKObject()))
  {
    vtkReflection::Set(algorithm, "SetAbortExecute", std::vector<vtkVariant>{false});
  }

  for (auto& el : state.at("properties").items())
  {
    const auto& key = el.key();
    const auto& value = el.value();
    assert(internals.Properties.find(key) != internals.Properties.end());
    const auto& pinfo = internals.Properties.at(key);
    if (value.find("elements") == value.end())
    {
      // the property has no value (not same as empty value)
      continue;
    }
    if (pinfo.Method.empty())
    {
      // property has no command -- nothing to update on the VTK object.
      continue;
    }
    switch (pinfo.Type)
    {
      case vtkSMProperty::INT:
      {
        auto values = value.at("elements").get<std::vector<int>>();
        vtkReflection::Set(internals.VTKObject, pinfo.Method, values);
      }
      break;

      case vtkSMProperty::DOUBLE:
      {
        auto values = value.at("elements").get<std::vector<double>>();
        vtkReflection::Set(internals.VTKObject, pinfo.Method, values);
      }
      break;

      case vtkSMProperty::ID_TYPE:
      {
        auto values = value.at("elements").get<std::vector<vtkIdType>>();
        vtkReflection::Set(internals.VTKObject, pinfo.Method, values);
      }
      break;

      case vtkSMProperty::STRING:
      {
        auto objects = value.at("elements");
        std::vector<vtkVariant> values(objects.size());
        std::transform(objects.begin(), objects.end(), values.begin(), [](const vtkNJson& value) {
          if (value.type() == vtkNJson::value_t::string)
          {
            return vtkVariant(value.get<std::string>());
          }
          if (value.type() == vtkNJson::value_t::number_integer ||
            value.type() == vtkNJson::value_t::number_unsigned)
          {
            return vtkVariant(value.get<int64_t>());
          }
          if (value.type() == vtkNJson::value_t::number_float)
          {
            return vtkVariant(value.get<double>());
          }
          return vtkVariant();
        });
        vtkReflection::Set(internals.VTKObject, pinfo.Method, values);
      }
      break;

      case vtkSMProperty::PROXY:
      {
        auto objectStore = provider->GetObjectStore();
        auto gids = value.at("elements").get<std::vector<vtkTypeUInt32>>();
        // convert gids to vtkobjects
        std::vector<vtkObject*> values;
        values.reserve(gids.size());
        std::transform(
          gids.begin(), gids.end(), std::back_inserter(values), [&objectStore](vtkTypeUInt32 gid) {
            auto smartPtr = objectStore->FindObject(gid);
            auto* wrapper = vtkObjectWrapper::SafeDownCast(smartPtr);
            return (wrapper ? wrapper->GetVTKObject() : nullptr);
          });
        vtkReflection::Set(internals.VTKObject, pinfo.Method, values);
      }
      break;

      case vtkSMProperty::INPUT:
      {
        auto objectStore = provider->GetObjectStore();
        std::vector<vtkAlgorithmOutput*> values;
        for (const auto& json : value.at("elements"))
        {
          auto producer = objectStore->FindObject(json.at("producer").get<vtkTypeUInt32>());
          auto* wrapper = vtkObjectWrapper::SafeDownCast(producer);
          if (auto* algorithm =
                wrapper ? vtkAlgorithm::SafeDownCast(wrapper->GetVTKObject()) : nullptr)
          {
            values.push_back(algorithm->GetOutputPort(json.at("index").get<int>()));
          }
        }
        vtkReflection::Set(internals.VTKObject, pinfo.Method, values);
      }
      break;

      default:
        abort();
    }
  }

  if (!internals.PostPushCommand.empty())
  {
    vtkReflection::Set(internals.VTKObject, internals.PostPushCommand, std::vector<vtkVariant>{});
  }
}

//----------------------------------------------------------------------------
void vtkObjectWrapper::InvokeCommand(const std::string& command)
{
  const auto& internals = (*this->Internals);
  auto iter = internals.Commands.find(command);
  auto* object = this->GetVTKObject();
  if (object && iter != internals.Commands.end())
  {
    vtkReflection::Set(object, iter->second, std::vector<vtkVariant>{});
  }
  else
  {
    vtkLogF(ERROR, "Unknown command '%s'", command.c_str());
  }
}

//----------------------------------------------------------------------------
vtkNJson vtkObjectWrapper::UpdateInformation(vtkRemoteObjectProvider* provider) const
{
  const auto& internals = (*this->Internals);
  auto* object = this->GetVTKObject();
  if (auto* algorithm = vtkAlgorithm::SafeDownCast(object))
  {
    vtkLogF(INFO, "Calling vtkAlgorithm::UpdateInformation");
    algorithm->UpdateInformation();
  }

  // TODO: ASYNC
  // scan all information properties and return their state.
  vtkNJson json = vtkNJson::object();
  json["status"] = true;
  vtkNJson properties = vtkNJson::object();
  std::vector<vtkVariant> args;
  for (const auto& item : internals.Properties)
  {
    const PropertyInformation propertyInfo = item.second;
    if (propertyInfo.InformationOnly)
    {
      vtkNJson property = vtkNJson::object();
      const std::string& propertyName = item.first;
      args.clear();
      property["name"] = propertyName;
      property["type"] = propertyInfo.Type;
      bool success = vtkReflection::Get(object, propertyInfo.Method, args);
      vtkNJson elements = vtkNJson::array();
      if (!success)
      {
        vtkLogF(WARNING, "Error getting values of property '%s' using '%s::%s' ",
          propertyName.c_str(), object->GetClassName(), propertyInfo.Method.c_str());
      }
      else
      {
        switch (propertyInfo.Type)
        {
          case vtkSMProperty::INT:
          {
            std::transform(args.begin(), args.end(), std::back_inserter(elements),
              [](const vtkVariant& v) { return vtk::variant_cast<int>(v); });
          }
          break;

          case vtkSMProperty::DOUBLE:
          {
            std::transform(args.begin(), args.end(), std::back_inserter(elements),
              [](const vtkVariant& v) { return vtk::variant_cast<double>(v); });
          }
          break;

          case vtkSMProperty::ID_TYPE:
          {
            std::transform(args.begin(), args.end(), std::back_inserter(elements),
              [](const vtkVariant& v) { return vtk::variant_cast<int>(v); });
          }
          break;

          case vtkSMProperty::STRING:
          {
            std::transform(args.begin(), args.end(), std::back_inserter(elements),
              [](const vtkVariant& v) { return vtk::variant_cast<std::string>(v); });
          }
          break;

          case vtkSMProperty::PROXY:
          {
            std::transform(args.begin(), args.end(), std::back_inserter(elements),
              [&provider](const vtkVariant& v) {
                auto* object = vtk::variant_cast_to_VTKObject<vtkObject*>(v);
                return provider->GetObjectStore()->GetID(object);
              });
          }
          break;

          case vtkSMProperty::INPUT:
          {
            std::transform(args.begin(), args.end(), std::back_inserter(elements),
              [&provider](const vtkVariant& v) {
                vtkNJson value = vtkNJson::object();
                auto* output = vtk::variant_cast_to_VTKObject<vtkAlgorithmOutput*>(v);
                assert(output != nullptr);
                value["producer"] = provider->GetObjectStore()->GetID(output->GetProducer());
                value["index"] = output->GetIndex();
                return value;
              });
          }
          break;
          default:
            abort();
        }
      }
      property["elements"] = elements;
      properties[propertyName] = property;
    }
  }
  json["properties"] = properties;

  return json;
}

//----------------------------------------------------------------------------
bool vtkObjectWrapper::UpdatePipeline(double time) const
{
  auto& internals = (*this->Internals);
  auto* object = this->GetVTKObject();
  if (auto* algorithm = vtkAlgorithm::SafeDownCast(object))
  {
    vtkLogF(INFO, "Calling vtkAlgorithm::UpdatePipeline(%f)", time);
    return algorithm->UpdateTimeStep(time, internals.Rank, internals.NumberOfRanks);
  }

  return false;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkObjectWrapper::GetGlobalID() const
{
  const auto& internals = (*this->Internals);
  return internals.GlobalID;
}

//----------------------------------------------------------------------------
vtkObject* vtkObjectWrapper::GetVTKObject() const
{
  const auto& internals = (*this->Internals);
  return internals.VTKObject;
}

//----------------------------------------------------------------------------
void vtkObjectWrapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
