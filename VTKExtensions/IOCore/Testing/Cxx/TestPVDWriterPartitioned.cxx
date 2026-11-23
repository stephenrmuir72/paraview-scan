// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataAssembly.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPVDReader.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPVDWriter.h"

#include <iostream>
#include <string>

#define TASSERT(x)                                                                                 \
  if (!(x))                                                                                        \
  {                                                                                                \
    std::cerr << "ERROR: failed at " << __LINE__ << "!" << endl;                                   \
    return EXIT_FAILURE;                                                                           \
  }

namespace
{
vtkSmartPointer<vtkImageData> MakeImage(int size)
{
  vtkNew<vtkImageData> image;
  image->SetDimensions(size, size, size);
  return image;
}

vtkSmartPointer<vtkDataObject> WriteAndRead(vtkDataObject* input, const std::string& fileName)
{
  vtkNew<vtkXMLPVDWriter> writer;
  writer->SetInputDataObject(input);
  writer->SetFileName(fileName.c_str());
  if (!writer->Write())
  {
    return nullptr;
  }

  vtkNew<vtkPVDReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  return reader->GetOutputDataObject(0);
}
}

extern int TestPVDWriterPartitioned(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  const std::string prefix = std::string(tempDir) + "/TestPVDWriterPartitioned";
  delete[] tempDir;

  // A partitioned dataset with two partitions.
  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetPartition(0, MakeImage(2));
  pds->SetPartition(1, MakeImage(3));

  vtkSmartPointer<vtkPartitionedDataSet> pdsOutput =
    vtkPartitionedDataSet::SafeDownCast(WriteAndRead(pds, prefix + "PDS.pvd"));
  TASSERT(pdsOutput != nullptr);
  TASSERT(pdsOutput->GetNumberOfPartitions() == 2);
  TASSERT(pdsOutput->GetNumberOfPoints() == pds->GetNumberOfPoints());
  TASSERT(pdsOutput->GetPartition(1)->GetNumberOfPoints() == MakeImage(3)->GetNumberOfPoints());

  // A collection with an assembly.
  vtkNew<vtkPartitionedDataSetCollection> pdc;
  pdc->SetPartition(0, 0, MakeImage(2));
  pdc->SetPartition(1, 0, MakeImage(2));
  pdc->SetPartition(1, 1, MakeImage(3));
  pdc->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "pair");
  vtkNew<vtkDataAssembly> assembly;
  assembly->AddDataSetIndex(vtkDataAssembly::GetRootNode(), 0);
  const int node = assembly->AddNode("group");
  assembly->AddDataSetIndex(node, 1);
  pdc->SetDataAssembly(assembly);

  vtkSmartPointer<vtkPartitionedDataSetCollection> pdcOutput =
    vtkPartitionedDataSetCollection::SafeDownCast(WriteAndRead(pdc, prefix + "PDC.pvd"));
  TASSERT(pdcOutput != nullptr);
  TASSERT(pdcOutput->GetNumberOfPartitionedDataSets() == 2);
  TASSERT(pdcOutput->GetNumberOfPartitions(0) == 1);
  TASSERT(pdcOutput->GetNumberOfPartitions(1) == 2);
  TASSERT(pdcOutput->GetNumberOfPoints() == pdc->GetNumberOfPoints());
  TASSERT(pdcOutput->HasMetaData(1u) &&
    std::string(pdcOutput->GetMetaData(1u)->Get(vtkCompositeDataSet::NAME())) == "pair");
  TASSERT(pdcOutput->GetDataAssembly() != nullptr);
  const int outputNode = pdcOutput->GetDataAssembly()->FindFirstNodeWithName("group");
  TASSERT(outputNode != -1);
  const auto indices = pdcOutput->GetDataAssembly()->GetDataSetIndices(outputNode);
  TASSERT(indices.size() == 1 && indices[0] == 1);
  return EXIT_SUCCESS;
}
