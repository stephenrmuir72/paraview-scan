// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Verifies that vtkIntersectFragments accepts vtkMultiBlockDataSet or
// vtkPartitionedDataSetCollection on its two input ports and produces an
// output of the same composite data type as the corresponding input.
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkIntersectFragments.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

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

vtkSmartPointer<vtkPolyData> MakeFragment(double cx, double cy, double cz)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(cx, cy, cz);
  sphere->SetRadius(0.6);
  sphere->Update();
  vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
  pd->DeepCopy(sphere->GetOutput());
  return pd;
}

// One material (block) with two fragments (pieces), each a small sphere
// straddling the x=0 cut plane.
vtkSmartPointer<vtkMultiBlockDataSet> MakeGeomMultiBlock()
{
  vtkSmartPointer<vtkMultiBlockDataSet> mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mb->SetNumberOfBlocks(1);
  vtkNew<vtkMultiPieceDataSet> mp;
  mp->SetNumberOfPieces(2);
  mp->SetPiece(0, MakeFragment(-0.3, 0, 0));
  mp->SetPiece(1, MakeFragment(0.3, 0, 0));
  mb->SetBlock(0, mp);
  return mb;
}

// One material (block) whose statistics are a single vtkPolyData with one
// point per fragment.
vtkSmartPointer<vtkMultiBlockDataSet> MakeStatsMultiBlock()
{
  vtkSmartPointer<vtkMultiBlockDataSet> mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mb->SetNumberOfBlocks(1);
  vtkNew<vtkPolyData> pd;
  vtkNew<vtkPoints> pts;
  pts->SetNumberOfPoints(2);
  pts->SetPoint(0, -1, 0, 0);
  pts->SetPoint(1, 1, 0, 0);
  pd->SetPoints(pts);
  vtkNew<vtkDoubleArray> volume;
  volume->SetName("Volume");
  volume->SetNumberOfTuples(2);
  volume->SetValue(0, 1.0);
  volume->SetValue(1, 2.0);
  pd->GetPointData()->AddArray(volume);
  mb->SetBlock(0, pd);
  return mb;
}

// Converts a vtkMultiBlockDataSet into the equivalent vtkPartitionedDataSetCollection.
vtkSmartPointer<vtkPartitionedDataSetCollection> ConvertToPDC(vtkMultiBlockDataSet* mb)
{
  vtkNew<vtkConvertToPartitionedDataSetCollection> converter;
  converter->SetInputDataObject(mb);
  converter->Update();
  return converter->GetOutput();
}

vtkSmartPointer<vtkImplicitFunction> MakeCutPlane()
{
  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0, 0, 0);
  plane->SetNormal(1, 0, 0);
  return plane;
}

void TestMultiBlockInOut()
{
  vtkNew<vtkIntersectFragments> filter;
  filter->SetCutFunction(MakeCutPlane());
  vtkSmartPointer<vtkMultiBlockDataSet> geomIn = MakeGeomMultiBlock();
  vtkSmartPointer<vtkMultiBlockDataSet> statsIn = MakeStatsMultiBlock();
  filter->SetInputDataObject(0, geomIn);
  filter->SetInputDataObject(1, statsIn);
  filter->Update();

  auto geomOut = vtkMultiBlockDataSet::SafeDownCast(filter->GetOutputDataObject(0));
  auto statsOut = vtkMultiBlockDataSet::SafeDownCast(filter->GetOutputDataObject(1));
  CHECK(geomOut != nullptr,
    "vtkMultiBlockDataSet geometry input should produce a "
    "vtkMultiBlockDataSet geometry output");
  CHECK(statsOut != nullptr,
    "vtkMultiBlockDataSet statistics input should produce a "
    "vtkMultiBlockDataSet statistics output");

  auto mp = geomOut ? vtkMultiPieceDataSet::SafeDownCast(geomOut->GetBlock(0)) : nullptr;
  CHECK(mp != nullptr, "geometry output block 0 should be a vtkMultiPieceDataSet");
  CHECK(mp && mp->GetNumberOfPieces() == 2, "geometry output should have 2 fragments");

  auto statsPd = statsOut ? vtkPolyData::SafeDownCast(statsOut->GetBlock(0)) : nullptr;
  CHECK(statsPd != nullptr, "statistics output block 0 should be a vtkPolyData");
  CHECK(statsPd && statsPd->GetNumberOfPoints() == 2, "statistics output should have 2 points");
}

void TestPartitionedDataSetCollectionInOut()
{
  vtkNew<vtkIntersectFragments> filter;
  filter->SetCutFunction(MakeCutPlane());
  vtkSmartPointer<vtkPartitionedDataSetCollection> geomIn = ConvertToPDC(MakeGeomMultiBlock());
  vtkSmartPointer<vtkPartitionedDataSetCollection> statsIn = ConvertToPDC(MakeStatsMultiBlock());
  filter->SetInputDataObject(0, geomIn);
  filter->SetInputDataObject(1, statsIn);
  filter->Update();

  auto geomOut = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));
  auto statsOut = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(1));
  CHECK(geomOut != nullptr,
    "vtkPartitionedDataSetCollection geometry input should produce a "
    "vtkPartitionedDataSetCollection geometry output");
  CHECK(statsOut != nullptr,
    "vtkPartitionedDataSetCollection statistics input should produce a "
    "vtkPartitionedDataSetCollection statistics output");

  CHECK(geomOut && geomOut->GetNumberOfPartitionedDataSets() == 1,
    "geometry output should have 1 partitioned dataset");
  CHECK(geomOut && geomOut->GetNumberOfPartitions(0) == 2,
    "geometry output partitioned dataset 0 should have 2 partitions");

  CHECK(statsOut && statsOut->GetNumberOfPartitionedDataSets() == 1,
    "statistics output should have 1 partitioned dataset");
  vtkDataSet* statsPoint = statsOut ? statsOut->GetPartition(0, 0) : nullptr;
  CHECK(statsPoint && statsPoint->GetNumberOfPoints() == 2,
    "statistics output partition (0,0) should have 2 points");
}

}

int TestIntersectFragmentsCompositeTypes(int, char*[])
{
  // vtkIntersectFragments uses vtkMultiProcessController::GetGlobalController()
  // and requires one to be set, even when running single-process.
  vtkNew<vtkDummyController> controller;
  vtkMultiProcessController::SetGlobalController(controller);

  TestMultiBlockInOut();
  TestPartitionedDataSetCollectionInOut();

  vtkMultiProcessController::SetGlobalController(nullptr);

  return NumErrors == 0 ? 0 : 1;
}
