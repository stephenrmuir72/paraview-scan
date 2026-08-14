// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Verifies that vtkPGenericIOMultiBlockWriter accepts a vtkMultiBlockDataSet
// or a vtkPartitionedDataSetCollection on its input, and writes equivalent
// GenericIO files from both. The writer expects each block on exactly one
// rank, so every rank holds the same blocks but only fills its own.
#include <vtk_mpi.h>

#include "vtkCommunicator.h"
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkFloatArray.h"
#include "vtkMPIController.h"
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

// Two blocks per rank, assigned round-robin. The blocks of other ranks stay
// null, so the block indices are the same on all ranks.
vtkSmartPointer<vtkMultiBlockDataSet> MakeMultiBlock(int rank, int numRanks)
{
  const unsigned int numBlocks = 2 * numRanks;
  vtkSmartPointer<vtkMultiBlockDataSet> mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mb->SetNumberOfBlocks(numBlocks);
  for (unsigned int i = rank; i < numBlocks; i += numRanks)
  {
    mb->SetBlock(i, MakeGrid(10.0 * i, 10.0f * i));
  }
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

vtkUnstructuredGrid* GetFirstLocalGrid(vtkMultiBlockDataSet* mb)
{
  for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i)
  {
    if (auto grid = vtkUnstructuredGrid::SafeDownCast(mb->GetBlock(i)))
    {
      return grid;
    }
  }
  return nullptr;
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

  const int rank = controller->GetLocalProcessId();
  const int numRanks = controller->GetNumberOfProcesses();

  vtkSmartPointer<vtkMultiBlockDataSet> mbInput = ::MakeMultiBlock(rank, numRanks);
  vtkSmartPointer<vtkPartitionedDataSetCollection> pdcInput = ::ConvertToPDC(mbInput);

  CHECK(::WriteFile(mbInput, mbFile), "writing from a vtkMultiBlockDataSet input should succeed");
  CHECK(::WriteFile(pdcInput, pdcFile),
    "writing from a vtkPartitionedDataSetCollection input should succeed");

  // Reading is collective, so all ranks must agree on whether to do it.
  int globalErrors = 0;
  controller->AllReduce(&NumErrors, &globalErrors, 1, vtkCommunicator::SUM_OP);
  if (globalErrors == 0)
  {
    vtkSmartPointer<vtkMultiBlockDataSet> mbReadBack = ::ReadFile(mbFile);
    vtkSmartPointer<vtkMultiBlockDataSet> pdcReadBack = ::ReadFile(pdcFile);

    CHECK(mbReadBack->GetNumberOfBlocks() == static_cast<unsigned int>(2 * numRanks),
      "the file written from a vtkMultiBlockDataSet input should read back with 2 blocks per "
      "rank");

    // Sanity-check actual values on the vtkMultiBlockDataSet-written file, so
    // that a bug producing two equally-wrong files would not go unnoticed.
    vtkUnstructuredGrid* firstGrid = ::GetFirstLocalGrid(mbReadBack);
    CHECK(firstGrid != nullptr, "each rank should read back a vtkUnstructuredGrid block");
    CHECK(firstGrid && firstGrid->GetNumberOfPoints() == 3,
      "the first local read-back block should have 3 points");
    auto firstId =
      firstGrid ? vtkFloatArray::SafeDownCast(firstGrid->GetPointData()->GetArray("id")) : nullptr;
    CHECK(
      firstId && firstId->GetNumberOfTuples() == 3, "the first block should have 3 'id' values");

    CHECK(vtkTestUtilities::CompareDataObjects(mbReadBack, pdcReadBack),
      "the files written from vtkMultiBlockDataSet and vtkPartitionedDataSetCollection inputs "
      "should read back identically");
  }

  controller->Finalize();

  return NumErrors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
