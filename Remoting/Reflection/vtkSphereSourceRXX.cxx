/*=========================================================================

  Program:   ParaView
  Module:    vtkSphereSourceRXX.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReflection.h"
#include "vtkSphereSource.h"

static bool vtkSphereSourceSetter(
  vtkObject* object, const std::string& method, const std::vector<vtkVariant>& args)
{
  auto* target = vtkSphereSource::SafeDownCast(object);
  if (!target)
  {
    return false;
  }

  if (method == "SetRadius" && args.size() == 1)
  {
    const double arg0 = vtk::variant_cast<double>(args[0]);
    vtkRLogF("vtkSphereSource(%p)::SetRadius(%f)", object, arg0);
    target->SetRadius(arg0);
    return true;
  }

  if (method == "SetCenter" && args.size() == 3)
  {
    const double arg0 = vtk::variant_cast<double>(args[0]);
    const double arg1 = vtk::variant_cast<double>(args[1]);
    const double arg2 = vtk::variant_cast<double>(args[2]);
    vtkRLogF("vtkSphereSource(%p)::SetCenter(%f, %f, %f)", object, arg0, arg1, arg2);
    target->SetCenter(arg0, arg1, arg2);
    return true;
  }

  if (method == "SetThetaResolution" && args.size() == 1)
  {
    const int arg0 = vtk::variant_cast<int>(args[0]);
    vtkRLogF("vtkSphereSource(%p)::SetThetaResolution(%d)", object, arg0);
    target->SetThetaResolution(arg0);
    return true;
  }

  if (method == "SetPhiResolution" && args.size() == 1)
  {
    const int arg0 = vtk::variant_cast<int>(args[0]);
    vtkRLogF("vtkSphereSource(%p)::SetPhiResolution(%d)", object, arg0);
    target->SetPhiResolution(arg0);
    return true;
  }

  if (method == "SetStartTheta" && args.size() == 1)
  {
    const double arg0 = vtk::variant_cast<double>(args[0]);
    vtkRLogF("vtkSphereSource(%p)::SetStartTheta(%f)", object, arg0);
    target->SetStartTheta(arg0);
    return true;
  }

  if (method == "SetEndTheta" && args.size() == 1)
  {
    const double arg0 = vtk::variant_cast<double>(args[0]);
    vtkRLogF("vtkSphereSource(%p)::SetEndTheta(%f)", object, arg0);
    target->SetEndTheta(arg0);
    return true;
  }

  if (method == "SetStartPhi" && args.size() == 1)
  {
    const double arg0 = vtk::variant_cast<double>(args[0]);
    vtkRLogF("vtkSphereSource(%p)::SetStartPhi(%f)", object, arg0);
    target->SetStartPhi(arg0);
    return true;
  }

  if (method == "SetEndPhi" && args.size() == 1)
  {
    const double arg0 = vtk::variant_cast<double>(args[0]);
    vtkRLogF("vtkSphereSource(%p)::SetEndPhi(%f)", object, arg0);
    target->SetEndPhi(arg0);
    return true;
  }

  return vtkReflection::Set(object, method, args, "vtkPolyDataAlgorithm");
}

static bool vtkSphereSourceGetter(
  vtkObject* object, const std::string& method, std::vector<vtkVariant>& args)
{
  auto* target = vtkSphereSource::SafeDownCast(object);
  if (!target)
  {
    return false;
  }

  if (method == "GetRadius")
  {
    args.emplace_back(target->GetRadius());
    return true;
  }

  return vtkReflection::Get(object, method, args, "vtkPolyDataAlgorithm");
}

void vtkSphereSourceRXX()
{
  vtkReflection::RegisterNew<vtkSphereSource>("vtkSphereSource");
  vtkReflection::RegisterSetter("vtkSphereSource", vtkSphereSourceSetter);
  vtkReflection::RegisterGetter("vtkSphereSource", vtkSphereSourceGetter);
}
