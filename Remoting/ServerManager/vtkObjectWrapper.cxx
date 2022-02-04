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

  // process properties.
  for (auto child : node.children())
  {
    if (vtkStringUtilities::EndsWith(child.name(), "Property"))
    {
      PropertyInformation info;
      info.Type = vtkSMProperty::GetPropertyType(fmt::format("vtkSM{}", child.name()));
      assert(info.Type != vtkSMProperty::INVALID);
      info.Method = child.attribute("command").as_string();
      info.InformationOnly = child.attribute("information_only").as_bool(false);
      internals.Properties.emplace(child.attribute("name").as_string(), std::move(info));
    }
    else if (strcmp(child.name(), "Command") == 0)
    {
      auto name = child.attribute("name").as_string();
      internals.Commands.emplace(name, child.attribute("method").as_string(name));
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkObjectWrapper::Initialize(
  vtkTypeUInt32 gid, const vtkJson& state, vtkRemoteObjectProvider* provider)
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
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkObjectWrapper::UpdateState(const vtkJson& state, vtkRemoteObjectProvider* provider)
{
  auto& internals = (*this->Internals);
  internals.LogName = state.at("log_name").get<std::string>();
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
        auto values = value.at("elements").get<std::vector<std::string>>();
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
vtkJson vtkObjectWrapper::UpdateInformation() const
{
  auto* object = this->GetVTKObject();
  if (auto* algorithm = vtkAlgorithm::SafeDownCast(object))
  {
    vtkLogF(INFO, "Calling vtkAlgorithm::UpdateInformation");
    algorithm->UpdateInformation();
  }

  // TODO: ASYNC
  // scan all information properties and return their state.
  return vtkJson{ { "status", true } };
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
