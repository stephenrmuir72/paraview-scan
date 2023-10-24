// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkESimMonChartExporter.h"
#include "vtkADIOS2DataArrayWriter.h"
#include "vtkAxis.h"
#include "vtkChart.h"
#include "vtkDataArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include "vtk_nlohmannjson.h"
// clang-format off
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

using vtkNJson = nlohmann::json;

class vtkESimMonChartExporter::vtkInternals
{
public:
  vtkNew<vtkADIOS2DataArrayWriter> Writer;
  vtkNJson GraphInformation;

  vtkInternals()
  {
    GraphInformation["type"] = "";
    GraphInformation["x"] = "";
    GraphInformation["y"] = vtkNJson::array();
    GraphInformation["yname"] = vtkNJson::array();
    GraphInformation["xlabel"] = "";
    GraphInformation["ylabel"] = "";
  }
};

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkESimMonChartExporter);

//----------------------------------------------------------------------------
vtkESimMonChartExporter::vtkESimMonChartExporter()
{
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkESimMonChartExporter::~vtkESimMonChartExporter()
{
  this->Close();
  delete this->Internals;
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void vtkESimMonChartExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkESimMonChartExporter::Open(ExporterModes mode)
{
  auto& internals = *(this->Internals);
  if (mode == ExporterModes::STREAM_COLUMNS)
  {
    internals.Writer->SetFileName(this->GetFileName());
    return internals.Writer->Open();
  }
  else
  {
    vtkWarningMacro("STREAM_ROW not yet implemented for vtkESimMonChartExporter");
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkESimMonChartExporter::Close()
{
  auto& internals = *(this->Internals);
  internals.Writer->AddAttribute(internals.GraphInformation.dump(), "plot");
  internals.Writer->Close();
}

//----------------------------------------------------------------------------
void vtkESimMonChartExporter::Abort()
{
  auto& internals = *(this->Internals);
  this->Close();
  vtksys::SystemTools::RemoveADirectory(this->FileName);
}

//----------------------------------------------------------------------------
void vtkESimMonChartExporter::WriteHeader(vtkFieldData*)
{
  vtkLogF(ERROR, "STREAM_ROWS mode is not supported for vtkESimMonChartExporter");
}
//----------------------------------------------------------------------------
void vtkESimMonChartExporter::WriteData(vtkFieldData*)
{
  vtkLogF(ERROR, "STREAM_ROWS mode is not supported for vtkESimMonChartExporter");
}

//----------------------------------------------------------------------------
void vtkESimMonChartExporter::AddColumn(
  vtkAbstractArray* yarray, const char* yarrayname, vtkDataArray* xarray)
{
  auto& internals = *(this->Internals);
  if (vtkDataArray* ydarray = vtkDataArray::SafeDownCast(yarray))
  {
    internals.Writer->AddArray(ydarray, yarrayname);
    internals.GraphInformation["y"].emplace_back(ydarray->GetName());
    internals.GraphInformation["yname"].emplace_back(yarrayname);
    if (xarray && !internals.Writer->ArrayExists(xarray->GetName()))
    {
      internals.Writer->AddArray(xarray, xarray->GetName());
      internals.GraphInformation["x"] = xarray->GetName();
    }
  }
  else
  {
    vtkLogF(ERROR, "vtkESimMonChartExporter does not yet support %s\nSkipping ...",
      yarray->GetArrayTypeAsString());
  }
}

//----------------------------------------------------------------------------
void vtkESimMonChartExporter::AddStyle(vtkPlot* plot, const char* plotName) {}
//----------------------------------------------------------------------------
void vtkESimMonChartExporter::SetGlobalStyle(vtkChart* chart)
{
  auto& internals = *(this->Internals);
  internals.GraphInformation.at("type") = "lines";
  internals.GraphInformation.at("xlabel") = chart->GetAxis(0)->GetTitle();
  internals.GraphInformation.at("ylabel") = chart->GetAxis(1)->GetTitle();
}
