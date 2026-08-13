// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// Verifies that vtkMeanPowerSpectralDensity, vtkMergeReduceTables,
// vtkSoundQuantitiesCalculator and vtkProjectSpectrumMagnitude accept a
// vtkPartitionedDataSetCollection input in addition to a vtkMultiBlockDataSet
// input, and that both composite types produce the same result.
#include "vtkCellArray.h"
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkMeanPowerSpectralDensity.h"
#include "vtkMergeReduceTables.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProjectSpectrumMagnitude.h"
#include "vtkSmartPointer.h"
#include "vtkSoundQuantitiesCalculator.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

#include <cmath>
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

// One table per mesh point, with a distinct signal per point (block index)
// so that per-point outputs are non-degenerate.
vtkSmartPointer<vtkTable> MakeTable(int blockIndex, vtkIdType nRows)
{
  vtkNew<vtkDoubleArray> freq;
  freq->SetName("Frequency");
  freq->SetNumberOfValues(nRows);

  vtkNew<vtkDoubleArray> fft;
  fft->SetName("FFT");
  fft->SetNumberOfValues(nRows);

  vtkNew<vtkDoubleArray> signal;
  signal->SetName("Signal");
  signal->SetNumberOfValues(nRows);

  vtkNew<vtkDoubleArray> pressure;
  pressure->SetName("Pressure");
  pressure->SetNumberOfValues(nRows);

  for (vtkIdType row = 0; row < nRows; ++row)
  {
    freq->SetValue(row, 10.0 * row);
    // Row 0 is treated as the DC frequency and ignored by
    // vtkMeanPowerSpectralDensity, so give it a distinctive value.
    fft->SetValue(row, row == 0 ? 5.0 : (blockIndex + row + 1.0));
    signal->SetValue(row, blockIndex * 10.0 + row);
    pressure->SetValue(row, (blockIndex + 1) * std::sin(0.5 * row + blockIndex));
  }

  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  table->AddColumn(freq);
  table->AddColumn(fft);
  table->AddColumn(signal);
  table->AddColumn(pressure);
  return table;
}

vtkSmartPointer<vtkMultiBlockDataSet> BuildMultiBlock(unsigned int nBlocks, vtkIdType nRows)
{
  vtkSmartPointer<vtkMultiBlockDataSet> mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  mb->SetNumberOfBlocks(nBlocks);
  for (unsigned int i = 0; i < nBlocks; ++i)
  {
    mb->SetBlock(i, MakeTable(static_cast<int>(i), nRows));
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

// A small two-triangle quad with 4 points, one per input table/block.
vtkSmartPointer<vtkPolyData> MakeMesh()
{
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  points->SetPoint(0, 0, 0, 0);
  points->SetPoint(1, 1, 0, 0);
  points->SetPoint(2, 1, 1, 0);
  points->SetPoint(3, 0, 1, 0);

  vtkNew<vtkCellArray> polys;
  const vtkIdType tri1[3] = { 0, 1, 2 };
  const vtkIdType tri2[3] = { 0, 2, 3 };
  polys->InsertNextCell(3, tri1);
  polys->InsertNextCell(3, tri2);

  vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
  mesh->SetPoints(points);
  mesh->SetPolys(polys);
  return mesh;
}

vtkSmartPointer<vtkTable> RunMeanPowerSpectralDensity(vtkDataObject* input)
{
  vtkNew<vtkMeanPowerSpectralDensity> filter;
  filter->SetInputDataObject(input);
  filter->SetFFTArrayName("FFT");
  filter->SetFrequencyArrayName("Frequency");
  filter->Update();

  vtkSmartPointer<vtkTable> output = vtkSmartPointer<vtkTable>::New();
  output->ShallowCopy(filter->GetOutput());
  return output;
}

vtkSmartPointer<vtkTable> RunMergeReduceTables(vtkDataObject* input)
{
  vtkNew<vtkMergeReduceTables> filter;
  filter->SetInputDataObject(input);
  filter->GetColumnToReduceSelection()->EnableArray("Signal");
  filter->GetOperationSelection()->EnableArray("Mean");
  filter->GetOperationSelection()->EnableArray("Sum");
  filter->Update();

  vtkSmartPointer<vtkTable> output = vtkSmartPointer<vtkTable>::New();
  output->ShallowCopy(filter->GetOutput());
  return output;
}

vtkSmartPointer<vtkDataSet> RunSoundQuantitiesCalculator(vtkDataObject* input, vtkDataSet* mesh)
{
  vtkNew<vtkSoundQuantitiesCalculator> filter;
  filter->SetInputDataObject(0, input);
  filter->SetSourceData(mesh);
  filter->SetPressureArrayName("Pressure");
  filter->Update();

  vtkSmartPointer<vtkDataSet> output = vtk::TakeSmartPointer(filter->GetOutput()->NewInstance());
  output->ShallowCopy(filter->GetOutput());
  return output;
}

vtkSmartPointer<vtkDataSet> RunProjectSpectrumMagnitude(vtkDataObject* input, vtkDataSet* mesh)
{
  vtkNew<vtkProjectSpectrumMagnitude> filter;
  filter->SetInputDataObject(0, input);
  filter->SetSourceData(mesh);
  filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "Frequency");
  filter->GetColumnSelection()->EnableArray("Signal");
  filter->SetLowerFrequency(0.0);
  filter->SetUpperFrequency(100.0);
  filter->Update();

  vtkSmartPointer<vtkDataSet> output = vtk::TakeSmartPointer(filter->GetOutput()->NewInstance());
  output->ShallowCopy(filter->GetOutput());
  return output;
}

}

int TestDSPFiltersCompositeTypes(int, char*[])
{
  // vtkMeanPowerSpectralDensity and vtkMergeReduceTables use
  // vtkMultiProcessController::GetGlobalController() and require one to be
  // set, even when running single-process.
  vtkNew<vtkDummyController> controller;
  vtkMultiProcessController::SetGlobalController(controller);

  const unsigned int nBlocks = 4;
  const vtkIdType nRows = 4;

  vtkSmartPointer<vtkMultiBlockDataSet> mb = ::BuildMultiBlock(nBlocks, nRows);
  vtkSmartPointer<vtkPartitionedDataSetCollection> pdc = ::ConvertToPDC(mb);
  vtkSmartPointer<vtkPolyData> mesh = ::MakeMesh();

  // vtkMeanPowerSpectralDensity
  {
    vtkSmartPointer<vtkTable> outMB = ::RunMeanPowerSpectralDensity(mb);
    vtkSmartPointer<vtkTable> outPDC = ::RunMeanPowerSpectralDensity(pdc);
    CHECK(outMB->GetColumnByName("Mean PSD (dB)") != nullptr,
      "vtkMeanPowerSpectralDensity should produce a 'Mean PSD (dB)' column");
    CHECK(vtkTestUtilities::CompareDataObjects(outMB, outPDC),
      "vtkMeanPowerSpectralDensity: vtkMultiBlockDataSet and "
      "vtkPartitionedDataSetCollection inputs should produce the same result");
  }

  // vtkMergeReduceTables
  {
    vtkSmartPointer<vtkTable> outMB = ::RunMergeReduceTables(mb);
    vtkSmartPointer<vtkTable> outPDC = ::RunMergeReduceTables(pdc);
    CHECK(outMB->GetColumnByName("Signal_Mean") != nullptr,
      "vtkMergeReduceTables should produce a 'Signal_Mean' column");
    CHECK(vtkTestUtilities::CompareDataObjects(outMB, outPDC),
      "vtkMergeReduceTables: vtkMultiBlockDataSet and vtkPartitionedDataSetCollection "
      "inputs should produce the same result");
  }

  // vtkSoundQuantitiesCalculator
  {
    vtkSmartPointer<vtkDataSet> outMB = ::RunSoundQuantitiesCalculator(mb, mesh);
    vtkSmartPointer<vtkDataSet> outPDC = ::RunSoundQuantitiesCalculator(pdc, mesh);
    CHECK(outMB->GetPointData()->GetArray("Mean Pressure (Pa)") != nullptr,
      "vtkSoundQuantitiesCalculator should produce a 'Mean Pressure (Pa)' point array");
    CHECK(vtkTestUtilities::CompareDataObjects(outMB, outPDC),
      "vtkSoundQuantitiesCalculator: vtkMultiBlockDataSet and "
      "vtkPartitionedDataSetCollection inputs should produce the same result");
  }

  // vtkProjectSpectrumMagnitude
  {
    vtkSmartPointer<vtkDataSet> outMB = ::RunProjectSpectrumMagnitude(mb, mesh);
    vtkSmartPointer<vtkDataSet> outPDC = ::RunProjectSpectrumMagnitude(pdc, mesh);
    CHECK(outMB->GetPointData()->GetArray("Signal") != nullptr,
      "vtkProjectSpectrumMagnitude should produce a 'Signal' point array");
    CHECK(vtkTestUtilities::CompareDataObjects(outMB, outPDC),
      "vtkProjectSpectrumMagnitude: vtkMultiBlockDataSet and "
      "vtkPartitionedDataSetCollection inputs should produce the same result");
  }

  vtkMultiProcessController::SetGlobalController(nullptr);

  return NumErrors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
