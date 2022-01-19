/*=========================================================================

  Program:   ParaView
  Module:    vtkShrinkFilterRXX.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReflection.h"
#include "vtkShrinkFilter.h"

static bool vtkShrinkFilterSetter(
  vtkObject* object, const std::string& method, const std::vector<vtkVariant>& args)
{
  auto* target = vtkShrinkFilter::SafeDownCast(object);
  if (!target)
  {
    return false;
  }

  if (method == "SetShrinkFactor" && args.size() == 1)
  {
    const double arg0 = vtk::variant_cast<double>(args[0]);
    vtkRLogF("vtkShrinkFilter(%p)::SetShrinkFactor(%f)", object, arg0);
    target->SetShrinkFactor(arg0);
    return true;
  }

  // this will really be implemented in `vtkAlgorithmRXX` but, I am doing it
  // here for now.
  if (method == "SetInputConnection" && args.size() == 1)
  {
    vtkAlgorithmOutput* arg0 = vtk::variant_cast<vtkAlgorithmOutput*>(args[0]);
    vtkRLogF("vtkShrinkFilter(%p)::SetInputConnection(%p)", object, arg0);
    target->SetInputConnection(arg0);
    return true;
  }

  return vtkReflection::Set(object, method, args, "vtkUnstructuredGridAlgorithm");
}

static bool vtkShrinkFilterGetter(
  vtkObject* object, const std::string& method, std::vector<vtkVariant>& args)
{
  return vtkReflection::Get(object, method, args, "vtkUnstructuredGridAlgorithm");
}

void vtkShrinkFilterRXX()
{
  vtkReflection::RegisterNew<vtkShrinkFilter>("vtkShrinkFilter");
  vtkReflection::RegisterSetter("vtkShrinkFilter", vtkShrinkFilterSetter);
  vtkReflection::RegisterGetter("vtkShrinkFilter", vtkShrinkFilterGetter);
}
