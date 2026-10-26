// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Verifies that vtkPANLHaloFinder and vtkPANLSubhaloFinder produce the same
// results when their particles come in a vtkUnstructuredGrid, a
// vtkMultiBlockDataSet or a vtkPartitionedDataSetCollection.
#include <vtk_mpi.h>

#include "vtkCompositeDataSet.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPANLHaloFinder.h"
#include "vtkPANLSubhaloFinder.h"
#include "vtkPGenericIOReader.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>

namespace
{
int NumErrors = 0;

#define CHECK(cond, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl;   \
      ++NumErrors;                                                                                 \
    }                                                                                              \
  } while (false)

enum class InputType
{
  UnstructuredGrid,
  MultiBlock,
  PartitionedDataSetCollection
};

const char* ToString(InputType type)
{
  switch (type)
  {
    case InputType::UnstructuredGrid:
      return "vtkUnstructuredGrid";
    case InputType::MultiBlock:
      return "vtkMultiBlockDataSet";
    default:
      return "vtkPartitionedDataSetCollection";
  }
}

// Wraps the particles in the requested type of dataset.
vtkSmartPointer<vtkDataObject> MakeInput(vtkUnstructuredGrid* particles, InputType type)
{
  if (type == InputType::MultiBlock)
  {
    auto mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    mb->SetBlock(0, particles);
    return mb;
  }
  if (type == InputType::PartitionedDataSetCollection)
  {
    auto pdc = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
    pdc->SetNumberOfPartitionedDataSets(1);
    pdc->SetPartition(0, 0, particles);
    return pdc;
  }
  return particles;
}

vtkIdType GetNumberOfPoints(vtkDataObject* data)
{
  if (auto composite = vtkCompositeDataSet::SafeDownCast(data))
  {
    return composite->GetNumberOfPoints();
  }
  auto grid = vtkUnstructuredGrid::SafeDownCast(data);
  return grid ? grid->GetNumberOfPoints() : -1;
}

vtkSmartPointer<vtkUnstructuredGrid> ReadParticles(int argc, char* argv[], bool haloTags)
{
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Testing/Data/genericio/m000.499.allparticles");

  vtkNew<vtkPGenericIOReader> reader;
  reader->SetFileName(fname);
  reader->UpdateInformation();
  reader->SetXAxisVariableName("x");
  reader->SetYAxisVariableName("y");
  reader->SetZAxisVariableName("z");
  reader->SetPointArrayStatus("vx", 1);
  reader->SetPointArrayStatus("vy", 1);
  reader->SetPointArrayStatus("vz", 1);
  reader->SetPointArrayStatus("id", 1);
  if (haloTags)
  {
    reader->SetPointArrayStatus("fof_halo_tag", 1);
  }
  reader->Update();
  delete[] fname;

  auto particles = vtkSmartPointer<vtkUnstructuredGrid>::New();
  particles->ShallowCopy(vtkUnstructuredGrid::SafeDownCast(reader->GetOutputDataObject(0)));
  return particles;
}

void TestHaloFinder(vtkUnstructuredGrid* particles)
{
  vtkIdType expectedPoints[2] = { -1, -1 };
  for (InputType type :
    { InputType::UnstructuredGrid, InputType::MultiBlock, InputType::PartitionedDataSetCollection })
  {
    vtkNew<vtkPANLHaloFinder> haloFinder;
    haloFinder->SetInputDataObject(MakeInput(particles, type));
    haloFinder->SetRL(128);
    haloFinder->SetParticleMass(13070871810);
    haloFinder->SetNP(128);
    haloFinder->SetPMin(100);
    haloFinder->Update();

    for (int port = 0; port < 2; ++port)
    {
      vtkIdType numberOfPoints = haloFinder->GetOutput(port)->GetNumberOfPoints();
      CHECK(numberOfPoints > 0,
        "vtkPANLHaloFinder output " << port << " is empty for " << ToString(type) << " input");
      if (type == InputType::UnstructuredGrid)
      {
        expectedPoints[port] = numberOfPoints;
      }
      CHECK(numberOfPoints == expectedPoints[port],
        "vtkPANLHaloFinder output " << port << " for " << ToString(type)
                                    << " input has a different number of points than for "
                                       "vtkUnstructuredGrid input");
    }
  }
}

void TestSubhaloFinder(vtkUnstructuredGrid* particles)
{
  vtkIdType expectedPoints[2] = { -1, -1 };
  for (InputType type :
    { InputType::UnstructuredGrid, InputType::MultiBlock, InputType::PartitionedDataSetCollection })
  {
    vtkNew<vtkPANLSubhaloFinder> subhaloFinder;
    subhaloFinder->SetInputDataObject(MakeInput(particles, type));
    subhaloFinder->SetRL(128);
    subhaloFinder->SetParticleMass(13070871810);
    subhaloFinder->SetBB(0.2);
    subhaloFinder->SetMinCandidateSize(20);
    subhaloFinder->AddHaloToProcess(1867323);
    subhaloFinder->SetMode(vtkPANLSubhaloFinder::ONLY_SELECTED_HALOS);
    subhaloFinder->Update();

    for (int port = 0; port < 2; ++port)
    {
      vtkDataObject* output = subhaloFinder->GetOutputDataObject(port);
      CHECK(output->IsA(ToString(type)),
        "vtkPANLSubhaloFinder output " << port << " should be a " << ToString(type) << ", got "
                                       << output->GetClassName());
      vtkIdType numberOfPoints = GetNumberOfPoints(output);
      CHECK(numberOfPoints > 0,
        "vtkPANLSubhaloFinder output " << port << " is empty for " << ToString(type) << " input");
      if (type == InputType::UnstructuredGrid)
      {
        expectedPoints[port] = numberOfPoints;
      }
      CHECK(numberOfPoints == expectedPoints[port],
        "vtkPANLSubhaloFinder output " << port << " for " << ToString(type)
                                       << " input has a different number of points than for "
                                          "vtkUnstructuredGrid input");
    }
  }
}
}

extern int TestHaloFinderCompositeTypes(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize();
  vtkMultiProcessController::SetGlobalController(controller.GetPointer());

  ::TestHaloFinder(::ReadParticles(argc, argv, false));
  ::TestSubhaloFinder(::ReadParticles(argc, argv, true));

  controller->Finalize();

  return NumErrors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
