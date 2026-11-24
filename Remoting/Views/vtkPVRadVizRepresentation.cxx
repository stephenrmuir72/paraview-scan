// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVRadVizRepresentation.h"

#include "vtkAbstractChartExporter.h"
#include "vtkChartRadViz.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPen.h"
#include "vtkPlotRadViz.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

class vtkPVRadVizRepresentation::vtkInternals
{
public:
  std::vector<std::pair<std::string, bool>> SeriesVisibilities;
  std::map<std::string, std::string> Labels;
  std::vector<int> PartitionIndices;
  std::vector<int> PartitionOffsets;
  std::vector<double> CustomRange;

  bool GetSeriesVisibility(const std::string& name)
  {
    for (size_t cc = 0; cc < this->SeriesVisibilities.size(); ++cc)
    {
      if (this->SeriesVisibilities[cc].first == name)
      {
        return this->SeriesVisibilities[cc].second;
      }
    }
    return false;
  }

  const std::string& GetLabel(const std::string& name)
  {
    auto iter = this->Labels.find(name);
    return iter != this->Labels.end() ? iter->second : name;
  }

  // nullptr for an unnamed column becomes ""
  static std::string GetColumnName(vtkTable* table, vtkIdType cc)
  {
    const char* name = table->GetColumnName(cc);
    return name ? name : "";
  }
};

vtkStandardNewMacro(vtkPVRadVizRepresentation);
//----------------------------------------------------------------------------
vtkPVRadVizRepresentation::vtkPVRadVizRepresentation()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkPVRadVizRepresentation::~vtkPVRadVizRepresentation()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVRadVizRepresentation::AddToView(vtkView* view)
{
  if (!this->Superclass::AddToView(view))
  {
    return false;
  }

  vtkChartRadViz* chart = this->GetChart();
  if (chart)
  {
    // the view's representations share the chart: only the visible one owns it.
    if (this->GetVisibility())
    {
      chart->SetVisible(true);
    }
    vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->GetPlot(0));
    if (plot)
    {
      this->PlotModifiedEventObserverId = plot->AddObserver(
        vtkCommand::ModifiedEvent, this, &vtkPVRadVizRepresentation::OnPlotModified);
      this->PlotStartInteractionObserverId = plot->AddObserver(vtkCommand::StartInteractionEvent,
        this, &vtkPVRadVizRepresentation::OnPlotStartInteraction);
      this->PlotEndInteractionObserverId = plot->AddObserver(
        vtkCommand::EndInteractionEvent, this, &vtkPVRadVizRepresentation::OnPlotEndInteraction);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPVRadVizRepresentation::RemoveFromView(vtkView* view)
{
  vtkChartRadViz* chart = this->GetChart();
  if (chart)
  {
    vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->GetPlot(0));
    if (plot)
    {
      if (this->PlotModifiedEventObserverId != 0)
      {
        plot->RemoveObserver(this->PlotModifiedEventObserverId);
      }
      if (this->PlotStartInteractionObserverId != 0)
      {
        plot->RemoveObserver(this->PlotStartInteractionObserverId);
      }
      if (this->PlotEndInteractionObserverId != 0)
      {
        plot->RemoveObserver(this->PlotEndInteractionObserverId);
      }
    }
    this->PlotModifiedEventObserverId = 0;
    this->PlotStartInteractionObserverId = 0;
    this->PlotEndInteractionObserverId = 0;

    if (this->GetVisibility())
    {
      if (plot)
      {
        plot->SetInputData(nullptr);
      }
      chart->SetVisible(false);
    }
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::OnPlotModified()
{
  // a hidden representation does not own the shared plot.
  if (this->IgnorePlotModified || !this->Interacting || !this->GetVisibility())
  {
    return;
  }

  vtkChartRadViz* chart = this->GetChart();
  vtkPlotRadViz* plot = chart ? vtkPlotRadViz::SafeDownCast(chart->GetPlot(0)) : nullptr;

  // The plot only reflects the properties once it has been rendered at least once.
  if (!plot || plot->GetSceneRadius() <= 0)
  {
    return;
  }

  bool changed = false;

  // the plot keeps the rotation in [0, 360): -10 and 350 are the same angle.
  double rotation = plot->GetVerticesRotation();
  double ownRotation = std::fmod(this->VerticesRotation, 360);
  if (ownRotation < 0)
  {
    ownRotation += 360;
  }
  // a tiny negative rotation rounds to 360, which the plot stores as 0
  if (ownRotation == 360)
  {
    ownRotation = 0;
  }
  if (rotation != ownRotation)
  {
    this->VerticesRotation = rotation;
    changed = true;
  }

  // both the plot and the properties store the lens in GBC space
  const double* lensCenter = plot->GetLensCenter();
  if (lensCenter[0] != this->LensCenter[0] || lensCenter[1] != this->LensCenter[1])
  {
    this->LensCenter[0] = lensCenter[0];
    this->LensCenter[1] = lensCenter[1];
    changed = true;
  }

  double lensRadius = plot->GetLensRadius();
  if (lensRadius != this->LensRadius)
  {
    this->LensRadius = lensRadius;
    changed = true;
  }

  if (changed)
  {
    this->InvokeEvent(vtkCommand::InteractionEvent);
  }
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::OnPlotStartInteraction()
{
  this->Interacting = true;
  if (this->GetVisibility())
  {
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
  }
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::OnPlotEndInteraction()
{
  this->Interacting = false;
  if (this->GetVisibility())
  {
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
  }
}

//----------------------------------------------------------------------------
vtkChartRadViz* vtkPVRadVizRepresentation::GetChart()
{
  if (this->ContextView)
  {
    return vtkChartRadViz::SafeDownCast(this->ContextView->GetContextItem());
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);

  vtkChartRadViz* chart = this->GetChart();
  if (chart && !visible)
  {
    // Refer to vtkChartRepresentation::PrepareForRendering() documentation to
    // know why this is cannot be done in PrepareForRendering();
    chart->SetVisible(false);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::SetSeriesVisibility(const char* series, bool visibility)
{
  assert(series != nullptr);
  this->Internals->SeriesVisibilities.emplace_back(series, visibility);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::ClearSeriesVisibilities()
{
  this->Internals->SeriesVisibilities.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::SetLabel(const char* series, const char* label)
{
  assert(series != nullptr && label != nullptr);
  this->Internals->Labels[series] = label;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::ClearLabels()
{
  this->Internals->Labels.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::ClearPartitionIndices()
{
  this->Internals->PartitionIndices.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::AddPartitionIndex(int index)
{
  this->Internals->PartitionIndices.push_back(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::ClearPartitionOffsets()
{
  this->Internals->PartitionOffsets.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::AddPartitionOffset(int offset)
{
  this->Internals->PartitionOffsets.push_back(offset);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::ClearCustomRange()
{
  this->Internals->CustomRange.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::AddCustomRange(double min, double max)
{
  this->Internals->CustomRange.push_back(min);
  this->Internals->CustomRange.push_back(max);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRadVizRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkChartRadViz* chart = this->GetChart();
  if (!chart)
  {
    return;
  }
  vtkTable* plotInput = this->GetLocalOutput();
  chart->SetVisible(plotInput != nullptr && this->GetVisibility());

  vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->GetPlot(0));
  if (plot)
  {
    // See IgnorePlotModified's documentation: the pushes below can
    // synchronously trigger OnPlotModified() reentrancy that would clobber
    // properties not yet pushed.
    this->IgnorePlotModified = true;

    plot->GetPen()->SetWidth(this->MarkerSize);
    plot->GetPen()->SetColorF(this->Color[0], this->Color[1], this->Color[2]);
    plot->SetScalarVisibility(this->ScalarVisibility);
    plot->SetColorWheelVisibility(this->ColorWheelVisibility);
    plot->SetColorWheelSegmentCount(this->ColorWheelSegmentCount);
    plot->SetVerticesRotation(this->VerticesRotation);
    plot->SetVerticesClockwiseOrdering(this->VerticesClockwiseOrdering);
    plot->SetLensVisibility(this->LensVisibility);
    plot->SetOutsideLensAlpha(this->OutsideLensAlpha);
    plot->SetPartitionIndices(this->Internals->PartitionIndices);
    plot->SetPartitionOffsets(this->Internals->PartitionOffsets);
    plot->SetCustomRange(this->Internals->CustomRange);

    plot->SetLensCenter(this->LensCenter);
    plot->SetLensRadius(this->LensRadius);

    this->IgnorePlotModified = false;

    if (plotInput)
    {
      // only the first vtkTable: Individual Blocks mode is not supported
      plot->SetInputData(plotInput);
      // by index: VTK renames unnamed and duplicate columns, so names would not match
      vtkIdType numCols = plotInput->GetNumberOfColumns();
      std::vector<int> visibleIndices;
      vtkSmartPointer<vtkStringArray> labels = vtkSmartPointer<vtkStringArray>::New();
      for (vtkIdType cc = 0; cc < numCols; cc++)
      {
        std::string name = vtkInternals::GetColumnName(plotInput, cc);
        if (this->Internals->GetSeriesVisibility(name))
        {
          visibleIndices.push_back(static_cast<int>(cc));
        }
        labels->InsertNextValue(this->Internals->GetLabel(name));
      }
      chart->SetVisibleVariableIndices(visibleIndices);
      plot->SetLabels(labels);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkPVRadVizRepresentation::Export(vtkAbstractChartExporter* exporter)
{
  vtkChartRadViz* chart = this->GetChart();
  vtkTable* plotInput = this->GetLocalOutput();
  if (!chart || !plotInput || this->GetVisibility() == false)
  {
    return false;
  }
  for (int cc : chart->GetVisibleVariableIndices())
  {
    exporter->AddColumn(
      plotInput->GetColumn(cc), vtkInternals::GetColumnName(plotInput, cc).c_str());
  }
  return true;
}
