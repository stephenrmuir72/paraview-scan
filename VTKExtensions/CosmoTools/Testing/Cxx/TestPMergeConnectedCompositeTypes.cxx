// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Verifies that vtkPMergeConnected accepts a vtkMultiBlockDataSet or a
// vtkPartitionedDataSetCollection on its input, and produces an output of
// the same composite data type as the input.
#include <vtk_mpi.h>

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPMergeConnected.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkSmartPointer.h"
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

// A single tetrahedron cell (4 triangular faces, none shared) with a local
// RegionId of 0, a synthetic "Volumes" cell value, and matching point-data
// RegionIds. Since each block/partition has exactly one local region, the
// filter's global region numbering assigns block i the global id i.
vtkSmartPointer<vtkUnstructuredGrid> MakeTetrahedronGrid(double xOffset, float volume)
{
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  points->SetPoint(0, xOffset + 0.0, 0.0, 0.0);
  points->SetPoint(1, xOffset + 1.0, 0.0, 0.0);
  points->SetPoint(2, xOffset + 0.0, 1.0, 0.0);
  points->SetPoint(3, xOffset + 0.0, 0.0, 1.0);

  vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  ugrid->SetPoints(points);
  ugrid->Allocate(1);

  // Polyhedron face stream: (numCellFaces, numFace0Pts, id..., numFace1Pts, id...)
  const vtkIdType faces[4][3] = { { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } };
  vtkNew<vtkIdList> faceStream;
  faceStream->InsertNextId(4);
  for (const auto& face : faces)
  {
    faceStream->InsertNextId(3);
    for (vtkIdType id : face)
    {
      faceStream->InsertNextId(id);
    }
  }
  ugrid->InsertNextCell(VTK_POLYHEDRON, faceStream);

  vtkNew<vtkIdTypeArray> cellRegionId;
  cellRegionId->SetName("RegionId");
  cellRegionId->SetNumberOfValues(1);
  cellRegionId->SetValue(0, 0);
  ugrid->GetCellData()->AddArray(cellRegionId);

  vtkNew<vtkFloatArray> volumes;
  volumes->SetName("Volumes");
  volumes->SetNumberOfValues(1);
  volumes->SetValue(0, volume);
  ugrid->GetCellData()->AddArray(volumes);

  vtkNew<vtkIdTypeArray> pointRegionId;
  pointRegionId->SetName("RegionId");
  pointRegionId->SetNumberOfValues(4);
  for (vtkIdType i = 0; i < 4; ++i)
  {
    pointRegionId->SetValue(i, 0);
  }
  ugrid->GetPointData()->AddArray(pointRegionId);

  return ugrid;
}

vtkSmartPointer<vtkMultiBlockDataSet> MakeMultiBlock()
{
  vtkSmartPointer<vtkMultiBlockDataSet> mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mb->SetNumberOfBlocks(2);
  mb->SetBlock(0, MakeTetrahedronGrid(0.0, 5.0f));
  mb->SetBlock(1, MakeTetrahedronGrid(10.0, 7.0f));
  return mb;
}

// Converts a flat vtkMultiBlockDataSet of vtkUnstructuredGrids into the
// equivalent vtkPartitionedDataSetCollection, where each grid becomes the
// single partition of its own partitioned data set.
vtkSmartPointer<vtkPartitionedDataSetCollection> ConvertToPDC(vtkMultiBlockDataSet* mb)
{
  vtkSmartPointer<vtkPartitionedDataSetCollection> pdc =
    vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  pdc->SetNumberOfPartitionedDataSets(mb->GetNumberOfBlocks());
  for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i)
  {
    vtkDataObject* block = mb->GetBlock(i);
    pdc->SetNumberOfPartitions(i, block ? 1 : 0);
    if (block)
    {
      pdc->SetPartition(i, 0, block);
    }
  }
  return pdc;
}

// Checks the merged output for a single block/partition: one merged
// polyhedron cell with all 4 original faces (none were shared, since there
// is only one region), the expected global RegionId, and the expected
// summed volume.
void CheckMergedGrid(vtkUnstructuredGrid* ugrid, vtkIdType expectedRegionId, float expectedVolume)
{
  CHECK(ugrid != nullptr, "output block/partition should be a vtkUnstructuredGrid");
  if (!ugrid)
  {
    return;
  }

  CHECK(ugrid->GetNumberOfCells() == 1, "output grid should have exactly one merged cell");

  auto poly = vtkPolyhedron::SafeDownCast(ugrid->GetCell(0));
  CHECK(poly != nullptr, "the merged cell should be a vtkPolyhedron");
  CHECK(poly && poly->GetNumberOfFaces() == 4,
    "the merged cell should keep all 4 faces of the original tetrahedron, since none are shared");

  auto crid = vtkIdTypeArray::SafeDownCast(ugrid->GetCellData()->GetArray("RegionId"));
  CHECK(crid != nullptr, "output cell data should have a 'RegionId' array");
  CHECK(crid && crid->GetValue(0) == expectedRegionId,
    "the merged cell should have the expected global RegionId");

  auto vol = vtkFloatArray::SafeDownCast(ugrid->GetCellData()->GetArray("Volumes"));
  CHECK(vol != nullptr, "output cell data should have a 'Volumes' array");
  CHECK(vol && vol->GetValue(0) == expectedVolume,
    "the merged cell's volume should be the sum of volumes for its region");

  auto prid = vtkIdTypeArray::SafeDownCast(ugrid->GetPointData()->GetArray("RegionId"));
  CHECK(prid != nullptr, "output point data should have a 'RegionId' array");
  if (prid)
  {
    for (vtkIdType i = 0; i < prid->GetNumberOfTuples(); ++i)
    {
      CHECK(prid->GetValue(i) == expectedRegionId,
        "point RegionIds should be offset to the same global id as the cell");
    }
  }
}

void TestMultiBlockInOut()
{
  vtkNew<vtkPMergeConnected> filter;
  vtkSmartPointer<vtkMultiBlockDataSet> input = ::MakeMultiBlock();
  filter->SetInputDataObject(input);
  filter->Update();

  auto output = vtkMultiBlockDataSet::SafeDownCast(filter->GetOutputDataObject(0));
  CHECK(
    output != nullptr, "vtkMultiBlockDataSet input should produce a vtkMultiBlockDataSet output");
  if (!output)
  {
    return;
  }

  CHECK(output->GetNumberOfBlocks() == 2, "output should have 2 blocks");
  ::CheckMergedGrid(vtkUnstructuredGrid::SafeDownCast(output->GetBlock(0)), 0, 5.0f);
  ::CheckMergedGrid(vtkUnstructuredGrid::SafeDownCast(output->GetBlock(1)), 1, 7.0f);
}

void TestPartitionedDataSetCollectionInOut()
{
  vtkNew<vtkPMergeConnected> filter;
  vtkSmartPointer<vtkPartitionedDataSetCollection> input = ::ConvertToPDC(::MakeMultiBlock());
  filter->SetInputDataObject(input);
  filter->Update();

  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
  CHECK(output != nullptr,
    "vtkPartitionedDataSetCollection input should produce a "
    "vtkPartitionedDataSetCollection output");
  if (!output)
  {
    return;
  }

  CHECK(
    output->GetNumberOfPartitionedDataSets() == 2, "output should have 2 partitioned data sets");
  ::CheckMergedGrid(vtkUnstructuredGrid::SafeDownCast(output->GetPartition(0, 0)), 0, 5.0f);
  ::CheckMergedGrid(vtkUnstructuredGrid::SafeDownCast(output->GetPartition(1, 0)), 1, 7.0f);
}

}

extern int TestPMergeConnectedCompositeTypes(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize();
  vtkMultiProcessController::SetGlobalController(controller.GetPointer());

  ::TestMultiBlockInOut();
  ::TestPartitionedDataSetCollectionInOut();

  controller->Finalize();

  return NumErrors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
