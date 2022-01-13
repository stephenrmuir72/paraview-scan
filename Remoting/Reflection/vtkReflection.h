/*=========================================================================

  Program:   ParaView
  Module:    vtkReflection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkReflection
 * @brief
 *
 */

#ifndef vtkReflection_h
#define vtkReflection_h

#include "vtkLogger.h" // for vtkLogger
#include "vtkObject.h"
#include "vtkRemotingReflectionModule.h" // for exports
#include "vtkSmartPointer.h"             // for vtkSmartPointer
#include "vtkVariant.h"                  // for vtkVariant

#include <algorithm>  //for std::transform
#include <functional> // for std::function
#include <vector>     // for std::vector

class VTKREMOTINGREFLECTION_EXPORT vtkReflection : public vtkObject
{
public:
  static vtkReflection* New();
  vtkTypeMacro(vtkReflection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create an instance given the type name.
   * For this to work, the instantiator function must have been previously
   * registered using `RegisterNew`.
   */
  static vtkSmartPointer<vtkObject> CreateInstance(const std::string& name);

  static bool Set(vtkObject* object, const std::string& methodName,
    const std::vector<vtkVariant>& args, const std::string& className = {});

  template <typename T>
  static bool Set(vtkObject* object, const std::string& methodName, const std::vector<T>& args)
  {
    std::vector<vtkVariant> variantArgs(args.size());
    std::copy(args.begin(), args.end(), variantArgs.begin());
    return vtkReflection::Set(object, methodName, variantArgs);
  }

  static bool Set(
    vtkObject* object, const std::string& methodName, const std::vector<std::string>& args)
  {
    std::vector<vtkVariant> variantArgs(args.size());
    std::transform(args.begin(), args.end(), variantArgs.begin(),
      [](const std::string& str) { return vtkVariant(vtkStdString(str)); });
    return vtkReflection::Set(object, methodName, variantArgs);
  }

  static bool Get(vtkObject* object, const std::string& methodName, std::vector<vtkVariant>& args,
    const std::string& className = {});

  /**
   * Register an instantiator callback.
   */
  static void RegisterNew(const std::string& className, std::function<vtkObject*()> constructor);

  /**
   * Register an instantiator callback for a vtkObject using `vtkObject::New` as
   * the instantiator.
   */
  template <typename VTKObject>
  static void RegisterNew(const std::string& className)
  {
    vtkReflection::RegisterNew(className, []() { return VTKObject::New(); });
  }

  static void RegisterSetter(const std::string& className,
    std::function<bool(vtkObject*, const std::string&, const std::vector<vtkVariant>&)> setter);
  static void RegisterGetter(const std::string& className,
    std::function<bool(vtkObject*, const std::string&, std::vector<vtkVariant>&)> getter);

  /**
   * Verbosity to use when logging reflection related debug messages.
   */
  static vtkLogger::Verbosity GetLogVerbosity();

protected:
  vtkReflection();
  ~vtkReflection();

private:
  vtkReflection(const vtkReflection&) = delete;
  void operator=(const vtkReflection&) = delete;
};

namespace vtk
{
template <typename T>
T variant_cast(const vtkVariant& variant);

template <>
inline double variant_cast<double>(const vtkVariant& variant)
{
  bool valid = false;
  double result = variant.ToDouble(&valid);
  if (!valid)
  {
    throw std::runtime_error("invalid cast");
  }
  return result;
}

template <>
inline int variant_cast<int>(const vtkVariant& variant)
{
  bool valid = false;
  int result = variant.ToInt(&valid);
  if (!valid)
  {
    throw std::runtime_error("invalid cast");
  }
  return result;
}
}

#define vtkRLogF(...) vtkVLogF(vtkReflection::GetLogVerbosity(), __VA_ARGS__)

#endif
