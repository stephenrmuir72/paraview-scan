// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDelimitedTextReader.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkTable.h"

#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

#include <adios2.h>

// Read the bp file created by TesteSimMonExtractor.py and validate its contents again a csv dump of
// the plot.
int ValidateeSimMonExtractor(int argc, char* argv[])
{
  if (argc != 3)
  {
    vtkLogF(ERROR, "Wrong number of arguments");
    return EXIT_FAILURE;
  }

  std::string filepath = argv[1];
  std::string csv_filepath = argv[2];

  adios2::ADIOS adios;
  adios2::IO io = adios.DeclareIO("Input");
  adios2::Engine reader = io.Open(filepath, adios2::Mode::ReadRandomAccess);

  if (!reader)
  {
    vtkLogF(ERROR, "Could not open file");
    return EXIT_FAILURE;
  }

  adios2::Variable<float> wVariable = io.InquireVariable<float>("Wavelet data");
  if (!wVariable)
  {
    vtkLogF(ERROR, "Variable Wavelet data does not exist in %s", filepath.c_str());
    return EXIT_FAILURE;
  }
  std::vector<float> wData;
  reader.Get<float>(wVariable, wData, adios2::Mode::Sync);

  adios2::Variable<float> aVariable = io.InquireVariable<float>("arc_length");
  if (!wVariable)
  {
    vtkLogF(ERROR, "Variable arc_length does not exist in %s", filepath.c_str());
    return EXIT_FAILURE;
  }
  std::vector<float> aData;
  reader.Get<float>(aVariable, aData, adios2::Mode::Sync);

  // Now read the reference file

  vtkNew<vtkDelimitedTextReader> csvReader;
  csvReader->SetHaveHeaders(1);
  csvReader->SetFileName(csv_filepath.c_str());
  csvReader->DetectNumericColumnsOn();
  csvReader->SetFieldDelimiterCharacters(",");
  csvReader->Update();

  vtkTable* tab = csvReader->GetOutput();
  vtkDoubleArray* rtData = vtkDoubleArray::SafeDownCast(tab->GetColumnByName("RTData"));
  if (!rtData)
  {
    vtkLogF(ERROR, "Variable RTData does not exist in %s", csv_filepath.c_str());
    return EXIT_FAILURE;
  }
  vtkDoubleArray* arcLength = vtkDoubleArray::SafeDownCast(tab->GetColumnByName("arc_length"));
  if (!arcLength)
  {
    vtkLogF(ERROR, "Variable arc_length does not exist in %s", csv_filepath.c_str());
    return EXIT_FAILURE;
  }

  auto almost_equal = [](double a, double b, double eps = 1e-2) { return std::abs(a - b) < eps; };

  if (!std::equal(wData.begin(), wData.end(), rtData->Begin(), almost_equal))
  {
    vtkLogF(ERROR, "expected values in RTData array do not match");
    return EXIT_FAILURE;
  }

  if (!std::equal(aData.begin(), aData.end(), arcLength->Begin(), almost_equal))
  {
    vtkLogF(ERROR, "expected values in arc_length array do not match");
    return EXIT_FAILURE;
  }

  adios2::Attribute<std::string> attribute = io.InquireAttribute<std::string>("plot");
  if (!attribute)
  {
    vtkLogF(ERROR, "Attribute testString does not exist in %s", filepath.c_str());
    return EXIT_FAILURE;
  }

  const std::string testString =
    "{\"type\":\"lines\",\"x\":\"arc_length\",\"xlabel\":\"Y Axis "
    "\",\"y\":[\"RTData\"],\"ylabel\":\"X axis\",\"yname\":[\"Wavelet data\"]}";
  if (attribute.Data()[0] != testString)
  {
    vtkLogF(ERROR, "expected value in testString attribute is wrong");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
  return ValidateeSimMonExtractor(argc, argv);
}
