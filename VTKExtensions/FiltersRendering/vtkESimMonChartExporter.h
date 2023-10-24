// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkESimMonChartExporter
 * @brief   exporter used by certain views to export data into an ADIOS2 BP file
 * following the schema of the eSimMon dashboard. See
 * https://esimmon.readthedocs.io/en/latest/schema/ for more details about the
 * schema.
 *
 */

#ifndef vtkESimMonChartExporter_h
#define vtkESimMonChartExporter_h

#include "vtkAbstractChartExporter.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

#include <string> // needed for std::string

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkESimMonChartExporter
  : public vtkAbstractChartExporter
{
public:
  static vtkESimMonChartExporter* New();
  vtkTypeMacro(vtkESimMonChartExporter, vtkAbstractChartExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the filename for the file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  using vtkAbstractChartExporter::ExporterModes;
  /**
   * Open the file and set mode in which the exporter is operating.
   */
  bool Open(ExporterModes mode = STREAM_ROWS) override;

  /**
   * Closes the file cleanly. Call this at the end to close the file and dump
   * out any cached data.
   */
  void Close() override;

  /**
   * Same as Close except deletes the file, if created. This is useful to
   * interrupt the exporting on failure.
   */
  void Abort() override;

  ///@{
  /**
   * Not supported by vtkESimMonChartExporter
   */
  void WriteHeader(vtkFieldData*) override;
  void WriteData(vtkFieldData*) override;
  ///@}

  /**
   * Add a new plot using xarray as "x" values and yarray as "y" values.
   * The name of the plot will be yarrayname
   */
  void AddColumn(vtkAbstractArray* yarray, const char* yarrayname = nullptr,
    vtkDataArray* xarray = nullptr) override;

  /**
   * @brief Add the stringified \p attribute under then name \p attributeName in the Adios file.
   *
   */
  void AddStyle(vtkPlot* plot, const char* plotName) override;

  /**
   * @brief
   */
  void SetGlobalStyle(vtkChart* chart) override;

protected:
  vtkESimMonChartExporter();
  ~vtkESimMonChartExporter() override;

private:
  vtkESimMonChartExporter(const vtkESimMonChartExporter&) = delete;
  void operator=(const vtkESimMonChartExporter&) = delete;

  char* FileName = nullptr;
  class vtkInternals;
  vtkInternals* Internals;
};

#endif
