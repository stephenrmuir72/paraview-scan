/*=========================================================================

  Program:   ParaView
  Module:    vtkReflection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReflection.h"

#include "vtkObjectFactory.h"
#include <map>
#include <mutex>

namespace
{

struct vtkReflectionSingleton
{
  std::mutex NewCallbacksMutex;
  std::map<std::string, std::function<vtkObject*()>> NewCallbacks;

  mutable std::mutex SettersMutex;
  std::map<std::string,
    std::function<bool(vtkObject*, const std::string&, const std::vector<vtkVariant>&)>>
    Setters;

  mutable std::mutex GettersMutex;
  std::map<std::string,
    std::function<bool(vtkObject*, const std::string&, std::vector<vtkVariant>&)>>
    Getters;

  static vtkReflectionSingleton& GetInstance()
  {
    static vtkReflectionSingleton instance;
    return instance;
  }

  auto GetSetter(const std::string& name) const
  {
    std::lock_guard<std::mutex> lk(this->SettersMutex);
    auto iter = this->Setters.find(name);
    return iter != this->Setters.end()
      ? iter->second
      : std::function<bool(vtkObject*, const std::string&, const std::vector<vtkVariant>&)>{};
  }

  auto GetGetter(const std::string& name) const
  {
    std::lock_guard<std::mutex> lk(this->GettersMutex);
    auto iter = this->Getters.find(name);
    return iter != this->Getters.end()
      ? iter->second
      : std::function<bool(vtkObject*, const std::string&, std::vector<vtkVariant>&)>{};
  }
};

} // namespace {}

vtkStandardNewMacro(vtkReflection);
//----------------------------------------------------------------------------
vtkReflection::vtkReflection() = default;

//----------------------------------------------------------------------------
vtkReflection::~vtkReflection() = default;

//----------------------------------------------------------------------------
void vtkReflection::RegisterNew(
  const std::string& className, std::function<vtkObject*()> constructor)
{
  auto& instance = vtkReflectionSingleton::GetInstance();
  std::lock_guard<std::mutex> lk(instance.NewCallbacksMutex);
  instance.NewCallbacks.emplace(className, constructor);
}

//----------------------------------------------------------------------------
void vtkReflection::RegisterSetter(const std::string& className,
  std::function<bool(vtkObject*, const std::string&, const std::vector<vtkVariant>&)> setter)
{
  auto& instance = vtkReflectionSingleton::GetInstance();
  std::lock_guard<std::mutex> lk(instance.SettersMutex);
  instance.Setters.emplace(className, setter);
}

//----------------------------------------------------------------------------
void vtkReflection::RegisterGetter(const std::string& className,
  std::function<bool(vtkObject*, const std::string&, std::vector<vtkVariant>&)> getter)
{
  auto& instance = vtkReflectionSingleton::GetInstance();
  std::lock_guard<std::mutex> lk(instance.GettersMutex);
  instance.Getters.emplace(className, getter);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkObject> vtkReflection::CreateInstance(const std::string& name)
{
  auto& instance = vtkReflectionSingleton::GetInstance();
  std::lock_guard<std::mutex> lk(instance.NewCallbacksMutex);
  auto iter = instance.NewCallbacks.find(name);
  return iter != instance.NewCallbacks.end() ? vtk::TakeSmartPointer(iter->second()) : nullptr;
}

//----------------------------------------------------------------------------
bool vtkReflection::Set(vtkObject* object, const std::string& methodName,
  const std::vector<vtkVariant>& args, const std::string& className)
{
  if (!object)
  {
    return false;
  }
  auto& instance = vtkReflectionSingleton::GetInstance();
  if (auto callback =
        instance.GetSetter(className.empty() ? std::string(object->GetClassName()) : className))
  {
    return callback(object, methodName, args);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkReflection::Get(vtkObject* object, const std::string& methodName,
  std::vector<vtkVariant>& args, const std::string& className)
{
  if (!object)
  {
    return false;
  }
  auto& instance = vtkReflectionSingleton::GetInstance();
  if (auto callback =
        instance.GetGetter(className.empty() ? std::string(object->GetClassName()) : className))
  {
    return callback(object, methodName, args);
  }
  return false;
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkReflection::GetLogVerbosity()
{
  return vtkLogger::VERBOSITY_INFO;
}

//----------------------------------------------------------------------------
void vtkReflection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
