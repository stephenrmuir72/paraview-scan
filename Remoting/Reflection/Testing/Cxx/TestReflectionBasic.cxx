/*=========================================================================

  Program:   ParaView
  Module:    TestReflectionBasic.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAlgorithmOutput.h"
#include "vtkLogger.h"
#include "vtkPVReflection.h"
#include "vtkReflection.h"

#include <cstdlib>

#define VALIDATE(x)                                                                                \
  if (!(x))                                                                                        \
  {                                                                                                \
    vtkLogF(ERROR, "Validating : (" #x ")...failed!");                                             \
    return EXIT_FAILURE;                                                                           \
  }

int TestReflectionBasic(int argc, char** argv)
{
  vtkNew<vtkPVReflection> Initializer;
  Initializer->Initialize();

  auto sphere = vtkReflection::CreateInstance("vtkSphereSource");
  VALIDATE(sphere != nullptr);

  bool success = vtkReflection::Set<int>(sphere, "SetThetaResolution", { 500 });
  VALIDATE(success);

  std::vector<vtkVariant> args;
  success = vtkReflection::Get(sphere, "GetThetaResolution", args);
  VALIDATE(success);
  VALIDATE(vtk::variant_cast<int>(args[0]) == 500);

  success = vtkReflection::Set(sphere, "SetCenter", { 1.0, 2.0, 3.0 });
  VALIDATE(success);

  args.clear();
  success = vtkReflection::Get(sphere, "GetCenter", args);
  VALIDATE(success);
  VALIDATE(vtk::variant_cast<double>(args[0]) == 1.0);
  VALIDATE(vtk::variant_cast<double>(args[1]) == 2.0);
  VALIDATE(vtk::variant_cast<double>(args[2]) == 3.0);

  auto reader = vtkReflection::CreateInstance("vtkPolyDataWriter");
  VALIDATE(reader != nullptr);

  std::string filename = "test.vtk";
  success = vtkReflection::Set(reader, "SetFileName", { filename });
  VALIDATE(success);

  args.clear();
  success = vtkReflection::Get(reader, "GetFileName", args);
  VALIDATE(success);
  VALIDATE(vtk::variant_cast<std::string>(args[0]) == std::string("test.vtk"));

  args.clear();
  success = vtkReflection::Get(sphere, "GetOutputPort", args);
  vtkSmartPointer<vtkAlgorithmOutput> out =
    vtk::variant_cast_to_VTKObject<vtkAlgorithmOutput*>(args[0]);

  success = vtkReflection::Set<vtkAlgorithmOutput*>(reader, "SetInputConnection", { out });
  VALIDATE(success);
  args.clear();
  success = vtkReflection::Set(reader, "Update", args);

  return EXIT_SUCCESS;
}
