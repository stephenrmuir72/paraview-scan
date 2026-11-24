// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVRadVizRepresentation
 *
 * vtkPVRadVizRepresentation is the vtkChartRadViz subclass for RadViz
 * representation. It exposes API from the underlying vtkChartRadViz and
 * vtkPlotRadViz.
 *
 * Only the MERGED_BLOCKS ArraySelectionMode is supported. In
 * INDIVIDUAL_BLOCKS mode, only the first block is plotted, as the
 * vtkChartRadViz has a single plot with a single input table.
 */

#ifndef vtkPVRadVizRepresentation_h
#define vtkPVRadVizRepresentation_h

#include "vtkChartRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkChartRadViz;

class VTKREMOTINGVIEWS_EXPORT vtkPVRadVizRepresentation : public vtkChartRepresentation
{
public:
  static vtkPVRadVizRepresentation* New();
  vtkTypeMacro(vtkPVRadVizRepresentation, vtkChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set visibility of the representation.
   */
  void SetVisibility(bool visible) override;

  ///@{
  /**
   * Set/clear series/columns visibility given its name.
   * vtkPVPlotMatrixRepresentation.
   */
  void SetSeriesVisibility(const char* series, bool visibility);
  void ClearSeriesVisibilities();
  ///@}

  ///@{
  /**
   * Set/clear the label used for a series/column, given its name. Used to
   * label the attribute vertex drawn for that series around the RadViz
   * circle instead of the column name. A series without an explicitly set
   * label falls back to using its column name.
   */
  void SetLabel(const char* series, const char* label);
  void ClearLabels();
  ///@}

  /**
   * Provides access to the underlying VTK representation.
   */
  vtkChartRadViz* GetChart();

  /**
   * Records the MultiVariateToRGBA filter this representation is
   * connected to (see the "MultiVariateSource" property and
   * pqRadVizChartView, which sets it up), so that the connection survives
   * being saved to, and reloaded from, a state file. Has no other effect:
   * this representation's live sync with the filter's properties is
   * driven entirely by pqRadVizChartView's property links, not by this
   * class.
   */
  void SetMultiVariateSource(vtkObjectBase*) {}

  ///@{
  /**
   * Sets the marker size used to draw the data points.
   */
  vtkSetMacro(MarkerSize, int);
  ///@}

  ///@{
  /**
   * Sets the color to use for the points when ScalarVisibility is off.
   */
  vtkSetVector3Macro(Color, double);
  ///@}

  ///@{
  /**
   * Turn on/off flag to control whether the points are colored using the
   * RGBA colors automatically computed by vtkMultiVariateToRGBA from the
   * multivariate data.
   */
  vtkSetMacro(ScalarVisibility, bool);
  ///@}

  ///@{
  /**
   * Show/hide the color wheel that helps interpret the colors computed
   * from the multivariate data.
   */
  vtkSetMacro(ColorWheelVisibility, bool);
  ///@}

  ///@{
  /**
   * Number of segments used when drawing the color wheel. More segments
   * produce a smoother gradient.
   */
  vtkSetMacro(ColorWheelSegmentCount, int);
  ///@}

  ///@{
  /**
   * Angle in degrees to rotate the vertices encoding the multivariate
   * attributes. Also updated when the user interactively rotates the
   * vertices in the view.
   */
  vtkSetMacro(VerticesRotation, double);
  vtkGetMacro(VerticesRotation, double);
  ///@}

  ///@{
  /**
   * Place the vertices clockwise if true, counter-clockwise otherwise.
   * The first vertex is at the top in both cases, before VerticesRotation
   * is applied.
   */
  vtkSetMacro(VerticesClockwiseOrdering, bool);
  ///@}

  ///@{
  /**
   * Show/hide the lens used to reduce the opacity of the points located
   * outside of it.
   */
  vtkSetMacro(LensVisibility, bool);
  ///@}

  ///@{
  /**
   * Alpha (opacity) cap for points outside the lens.
   */
  vtkSetMacro(OutsideLensAlpha, double);
  ///@}

  ///@{
  /**
   * Center of the lens, in normalized GBC space (unit circle centered at
   * the origin). The lens must intersect the GBC circle; the value is
   * clamped accordingly. Also updated when the user interactively drags
   * the lens in the view. @sa vtkMultiVariateToRGBA
   */
  vtkSetVector2Macro(LensCenter, double);
  vtkGetVector2Macro(LensCenter, double);
  ///@}

  ///@{
  /**
   * Radius of the lens, in normalized GBC space (unit circle centered at
   * the origin). The lens must intersect the GBC circle; the value is
   * clamped accordingly. Also updated when the user interactively resizes
   * the lens in the view. @sa vtkMultiVariateToRGBA
   */
  vtkSetMacro(LensRadius, double);
  vtkGetMacro(LensRadius, double);
  ///@}

  ///@{
  /**
   * Variables can be partitioned into several groups so that multivariate
   * scalars in each group are normalized together instead of over the
   * whole dataset. See vtkMultiVariateToRGBA::SetPartitionIndices for the
   * PartitionIndices/PartitionOffsets layout. Passed through to the
   * underlying vtkPlotRadViz used to render the chart.
   */
  void ClearPartitionIndices();
  void AddPartitionIndex(int index);
  void ClearPartitionOffsets();
  void AddPartitionOffset(int offset);
  ///@}

  ///@{
  /**
   * Custom [min, max] range used to normalize each partition group instead
   * of the range computed from the data. See
   * vtkMultiVariateToRGBA::SetCustomRange for the layout. ClearCustomRange()
   * empties it and AddCustomRange() appends the range of the next group.
   * Passed through to the underlying vtkPlotRadViz.
   */
  void ClearCustomRange();
  void AddCustomRange(double min, double max);
  ///@}

  ///@{
  /**
   * Called by vtkPVContextView::Export() to export the representation's data to
   * a CSV file. Return false on failure which will call the exporting process
   * to abort and raise an error. Default implementation simply returns false.
   */
  bool Export(vtkAbstractChartExporter* exporter) override;
  ///@}

protected:
  vtkPVRadVizRepresentation();
  ~vtkPVRadVizRepresentation() override;

  /**
   * Overridden to pass information about changes to the plot properties to
   * the RadViz chart.
   */
  void PrepareForRendering() override;

  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  /**
   * Called whenever the underlying vtkPlotRadViz is modified. During an
   * interactive vertices rotation or lens drag/resize in the view,
   * synchronizes VerticesRotation, LensCenter and LensRadius from the plot
   * so that the representation's properties reflect the interactive state.
   * Outside a drag, a difference comes from a value the plot rejected (e.g. a
   * negative LensRadius), so it is ignored.
   */
  void OnPlotModified();

  ///@{
  /**
   * Called when the plot's underlying vtkChartRadViz fires
   * vtkCommand::StartInteractionEvent/EndInteractionEvent, i.e. when the
   * user starts/stops interactively dragging the vertices or lens.
   * Re-fires the same event on this representation, so that e.g.
   * vtkSMRadVizRepresentationProxy can, in turn, let Qt-level code (see
   * pqRadVizChartView) tell a single interactive drag apart from the many
   * OnPlotModified()-driven InteractionEvents fired while it is in
   * progress.
   */
  void OnPlotStartInteraction();
  void OnPlotEndInteraction();
  ///@}

  int MarkerSize = 6;
  double Color[3] = { 1.0, 1.0, 1.0 };
  bool ScalarVisibility = true;
  bool ColorWheelVisibility = true;
  int ColorWheelSegmentCount = 120;
  double VerticesRotation = 0.0;
  bool VerticesClockwiseOrdering = true;
  bool LensVisibility = false;
  double OutsideLensAlpha = 0.0;
  double LensCenter[2] = { 0.0, 0.0 };
  double LensRadius = 1.0;

  unsigned long PlotModifiedEventObserverId = 0;
  unsigned long PlotStartInteractionObserverId = 0;
  unsigned long PlotEndInteractionObserverId = 0;

  /**
   * Set while PrepareForRendering() is pushing this representation's
   * properties onto the plot, so that OnPlotModified() ignores the
   * vtkCommand::ModifiedEvent(s) that those pushes themselves trigger on
   * the plot. Without this, e.g. SetVerticesRotation() can synchronously
   * re-enter OnPlotModified() before LensCenter/LensRadius have been
   * pushed, which would read back the plot's still-stale lens state and
   * clobber the very values PrepareForRendering() is about to set.
   */
  bool IgnorePlotModified = false;

  // True between the plot's StartInteractionEvent and EndInteractionEvent.
  bool Interacting = false;

private:
  vtkPVRadVizRepresentation(const vtkPVRadVizRepresentation&) = delete;
  void operator=(const vtkPVRadVizRepresentation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
