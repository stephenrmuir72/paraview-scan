// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Verifies that vtkPGenericIOMultiBlockWriter accepts a vtkMultiBlockDataSet
// or a vtkPartitionedDataSetCollection on its input, and writes equivalent
// GenericIO files from both.
#include <vtk_mpi.h>

#include "vtkFloatArray.h"
#include "vtkMPIController.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPGenericIOMultiBlockReader.h"
#include "vtkPGenericIOMultiBlockWriter.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <iostream>
#include <string>

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

// A point cloud with 3 points and a scalar "id" point array. The writer
// only reads points and point data, so no cells are needed.
vtkSmartPointer<vtkUnstructuredGrid> MakeGrid(double xOffset, float idStart)
{
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(3);
  for (vtkIdType i = 0; i < 3; ++i)
  {
    points->SetPoint(i, xOffset + static_cast<double>(i), 0.0, 0.0);
  }

  vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);

  vtkNew<vtkFloatArray> id;
  id->SetName("id");
  id->SetNumberOfValues(3);
  for (vtkIdType i = 0; i < 3; ++i)
  {
    id->SetValue(i, idStart + static_cast<float>(i));
  }
  grid->GetPointData()->AddArray(id);

  return grid;
}

vtkSmartPointer<vtkMultiBlockDataSet> MakeMultiBlock()
{
  vtkSmartPointer<vtkMultiBlockDataSet> mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mb->SetNumberOfBlocks(2);
  mb->SetBlock(0, MakeGrid(0.0, 0.0f));
  mb->SetBlock(1, MakeGrid(10.0, 10.0f));
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

bool WriteFile(vtkDataObject* input, const std::string& filename)
{
  vtkNew<vtkPGenericIOMultiBlockWriter> writer;
  writer->SetInputDataObject(input);
  writer->SetFileName(filename.c_str());
  return writer->Write() != 0;
}

vtkSmartPointer<vtkMultiBlockDataSet> ReadFile(const std::string& filename)
{
  vtkNew<vtkPGenericIOMultiBlockReader> reader;
  reader->SetFileName(filename.c_str());
  reader->UpdateInformation();
  reader->SetPointArrayStatus("id", 1);
  reader->Update();

  vtkSmartPointer<vtkMultiBlockDataSet> output = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  output->ShallowCopy(reader->GetOutput());
  return output;
}

bool CompareUnstructuredGrids(vtkUnstructuredGrid* a, vtkUnstructuredGrid* b)
{
  if (!a || !b)
  {
    std::cerr << "One of the grids to compare is null." << std::endl;
    return false;
  }
  if (a->GetNumberOfPoints() != b->GetNumberOfPoints())
  {
    std::cerr << "Point count mismatch: " << a->GetNumberOfPoints() << " vs "
              << b->GetNumberOfPoints() << std::endl;
    return false;
  }

  bool ok = true;
  for (vtkIdType i = 0; i < a->GetNumberOfPoints(); ++i)
  {
    double pa[3];
    double pb[3];
    a->GetPoint(i, pa);
    b->GetPoint(i, pb);
    for (int c = 0; c < 3; ++c)
    {
      if (!vtkMathUtilities::FuzzyCompare(pa[c], pb[c], 1e-6))
      {
        std::cerr << "Point " << i << " component " << c << " mismatch: " << pa[c] << " vs "
                  << pb[c] << std::endl;
        ok = false;
      }
    }
  }

  auto idA = vtkFloatArray::SafeDownCast(a->GetPointData()->GetArray("id"));
  auto idB = vtkFloatArray::SafeDownCast(b->GetPointData()->GetArray("id"));
  if (!idA || !idB)
  {
    std::cerr << "Missing 'id' array in one of the grids." << std::endl;
    return false;
  }
  if (idA->GetNumberOfTuples() != idB->GetNumberOfTuples())
  {
    std::cerr << "'id' array size mismatch." << std::endl;
    return false;
  }
  for (vtkIdType i = 0; i < idA->GetNumberOfTuples(); ++i)
  {
    if (!vtkMathUtilities::FuzzyCompare(idA->GetValue(i), idB->GetValue(i), 1e-4f))
    {
      std::cerr << "'id' value " << i << " mismatch: " << idA->GetValue(i) << " vs "
                << idB->GetValue(i) << std::endl;
      ok = false;
    }
  }
  return ok;
}

}

extern int TestGenericIOMultiBlockWriterCompositeTypes(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize();
  vtkMultiProcessController::SetGlobalController(controller.GetPointer());

  char* tempDirCStr = vtkTestUtilities::GetTemporaryDir(argc, argv);
  const std::string tempDir(tempDirCStr);
  delete[] tempDirCStr;

  const std::string mbFile = tempDir + "/TestGenericIOMultiBlockWriterMB.gio";
  const std::string pdcFile = tempDir + "/TestGenericIOMultiBlockWriterPDC.gio";

  vtkSmartPointer<vtkMultiBlockDataSet> mbInput = ::MakeMultiBlock();
  vtkSmartPointer<vtkPartitionedDataSetCollection> pdcInput = ::ConvertToPDC(mbInput);

  CHECK(::WriteFile(mbInput, mbFile), "writing from a vtkMultiBlockDataSet input should succeed");
  CHECK(::WriteFile(pdcInput, pdcFile),
    "writing from a vtkPartitionedDataSetCollection input should succeed");

  if (NumErrors == 0)
  {
    vtkSmartPointer<vtkMultiBlockDataSet> mbReadBack = ::ReadFile(mbFile);
    vtkSmartPointer<vtkMultiBlockDataSet> pdcReadBack = ::ReadFile(pdcFile);

    CHECK(mbReadBack->GetNumberOfBlocks() == 2,
      "the file written from a vtkMultiBlockDataSet input should read back with 2 blocks");
    CHECK(pdcReadBack->GetNumberOfBlocks() == mbReadBack->GetNumberOfBlocks(),
      "the files written from vtkMultiBlockDataSet and vtkPartitionedDataSetCollection inputs "
      "should read back with the same number of blocks");

    // Sanity-check actual values on the vtkMultiBlockDataSet-written file, so
    // that a bug producing two equally-wrong files would not go unnoticed.
    auto firstGrid = vtkUnstructuredGrid::SafeDownCast(mbReadBack->GetBlock(0));
    CHECK(firstGrid != nullptr, "the first read-back block should be a vtkUnstructuredGrid");
    CHECK(firstGrid && firstGrid->GetNumberOfPoints() == 3,
      "the first read-back block should have 3 points");
    auto firstId =
      firstGrid ? vtkFloatArray::SafeDownCast(firstGrid->GetPointData()->GetArray("id")) : nullptr;
    CHECK(
      firstId && firstId->GetNumberOfTuples() == 3, "the first block should have 3 'id' values");

    for (unsigned int i = 0;
         i < mbReadBack->GetNumberOfBlocks() && i < pdcReadBack->GetNumberOfBlocks(); ++i)
    {
      CHECK(::CompareUnstructuredGrids(vtkUnstructuredGrid::SafeDownCast(mbReadBack->GetBlock(i)),
              vtkUnstructuredGrid::SafeDownCast(pdcReadBack->GetBlock(i))),
        "block " << i
                 << " written from vtkMultiBlockDataSet and vtkPartitionedDataSetCollection "
                    "inputs should read back identically");
    }
  }

  controller->Finalize();

  return NumErrors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
