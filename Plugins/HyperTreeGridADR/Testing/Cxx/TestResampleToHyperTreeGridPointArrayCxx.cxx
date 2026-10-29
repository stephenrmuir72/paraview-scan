// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>
#include <vtkArithmeticMeanArrayMeasurement.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkHyperTreeGrid.h>
#include <vtkInformation.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkResampleToHyperTreeGrid.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>
#include <vtkXMLHyperTreeGridReader.h>

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

// ----------------------------------------------------------------------------
int TestResampleToHyperTreeGridPointArrayCxx(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPIController* contr = vtkMPIController::New();
#else
  vtkDummyController* contr = vtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(8, 2, -3);
  sphere->SetRadius(5);
  sphere->SetPhiResolution(10);
  sphere->SetThetaResolution(10);
  sphere->Update();

  vtkNew<vtkDoubleArray> doubleArray;
  doubleArray->SetNumberOfComponents(1);
  doubleArray->SetNumberOfValues(sphere->GetOutput()->GetNumberOfPoints());
  doubleArray->Fill(0.0);

  const char* arrayName = "SphereArray";
  doubleArray->SetName(arrayName);

  sphere->GetOutput()->GetPointData()->AddArray(doubleArray);
  sphere->Update();

  vtkNew<vtkResampleToHyperTreeGrid> resample;
  resample->SetInputConnection(sphere->GetOutputPort());
  resample->SetDimensions(10, 6, 4);
  resample->SetBranchFactor(3);
  resample->SetMaxDepth(1);
  resample->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, arrayName);
  resample->SetUpperThreshold(1);
  resample->SetLowerThreshold(0);
  vtkNew<vtkArithmeticMeanArrayMeasurement> arithmeticMean;
  resample->SetInterpolationMethod(arithmeticMean);
  resample->Update();

  vtkHyperTreeGrid* resampledHtg = vtkHyperTreeGrid::SafeDownCast(resample->GetOutputDataObject(0));

  if (!resampledHtg)
  {
    std::cout << "Failed to create HyperTreeGrid from vtkResampleToHyperTreeGrid filter"
              << std::endl;
    contr->Finalize();
    contr->Delete();
    return 1;
  }

  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/HTG/sphere_resampled.htg");

  // HTG reader
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;

  vtkHyperTreeGrid* readHtg = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  if (!readHtg)
  {
    std::cout << "Failed to read HyperTreeGrid from " << fileName << std::endl;
    contr->Finalize();
    contr->Delete();
    return 1;
  }

  if (!vtkTestUtilities::CompareDataObjects(readHtg, resampledHtg))
  {
    std::cout << "The generated HTG doesn't match the expected one" << std::endl;
    contr->Finalize();
    contr->Delete();
    return 1;
  }

  contr->Finalize();
  contr->Delete();
  return 0;
}
