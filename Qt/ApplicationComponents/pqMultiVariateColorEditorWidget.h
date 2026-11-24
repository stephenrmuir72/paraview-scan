// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqMultiVariateColorEditorWidget_h
#define pqMultiVariateColorEditorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyGroupWidget.h"

class vtkSMProxy;
class vtkSMPropertyGroup;
class pqPipelineSource;
class pqRepresentation;
class pqView;

/**
 * pqMultiVariateColorEditorWidget is a custom property-group widget
 * for the `MultiVariateToRGBA` filter's "Coloring" property group. It
 * provides controls for the `VerticesRotation`, `LensCenter`,
 * `LensRadius` and `InvertLens` properties. It also has a
 * 'Show RadViz' button which shows a RadViz view of the input to
 * the MultiVariateToRGBA filter and
 * allows the user to adjust the Coloring controls by interacting with the
 * view: rotate the vertices to modify VerticesRotation and drag and
 * resize the lens to change LensCenter and LensRadius. Editing
 * VerticesRotation in this widget, or rotating the vertices in the view,
 * also rotates LensCenter, so the lens keeps the same data points. The RadViz
 * representation's VerticesRotation, LensCenter and LensRadius properties
 * are kept in sync with the same-named properties of the MultiVariateToRGBA
 * filter in both directions by the RadViz view itself (see
 * pqRadVizChartView), not by this widget: editing this widget's controls
 * (or clicking Apply) pushes the new values to the RadViz proxy properties,
 * and the view live-updates the RadViz representation from them right
 * away; interacting with the RadViz view instead only updates the
 * MultiVariateToRGBA filter's properties (reflected in this widget's
 * controls) and highlights Apply, without pushing the change to the
 * filter's VTK object, so the RenderView showing the filter's output is
 * only updated once the user clicks Apply. Because this sync is owned by
 * the view rather than this widget, it keeps working even while no
 * pqMultiVariateColorEditorWidget exists for the filter (e.g. its
 * properties are not currently shown), including when the connection is
 * made or broken through the RadViz view's own right-click context menu.
 *
 * The RadViz view runs on the client, so its input data must be downloaded
 * there. When the 'Downsample' checkbox is checked (the default), a
 * `ResampleToImage` filter is created between the MultiVariateToRGBA
 * filter's input and the RadViz representation, downsampling the data onto
 * a small image before it reaches the view, to keep that download small
 * regardless of the size of the filter's input. `SamplingDimensions` are
 * proportional to the sides of the input's bounds, with 10 samples along
 * the longest side. When unchecked, no such filter is created and the
 * RadViz view shows the MultiVariateToRGBA filter's input directly,
 * unresampled.
 *
 * Once created, the RadViz view, its representation and the ResampleToImage
 * filter are ordinary pipeline/view objects: they are deleted through the
 * normal ParaView interface (the pipeline browser or the view's close
 * button), not by this widget. This widget only creates them on first use
 * and, if it is recreated for a MultiVariateToRGBA filter that already has
 * a RadViz representation set up (e.g. because the widget was rebuilt, or
 * the state was reloaded), it finds and reuses that existing representation
 * instead of creating a duplicate one. Since the representation's proxy or
 * the view can be destroyed at any time behind this widget's back, it
 * watches for their removal so it stops tracking them before either proxy
 * actually disappears.
 *
 * While the RadViz view is connected to the MultiVariateToRGBA filter, its
 * name (as shown in the pipeline browser) is annotated with that filter's
 * own pipeline browser name, in parenthesis, e.g. "RadVizChartView1
 * (MultiVariateToRGBA1)". pqRadVizChartView keeps the annotation in sync
 * when the filter is renamed, and removes it (leaving just the RadViz
 * view's own name) when the filter is deleted.
 *
 * @sa vtkMultiVariateToRGBA, vtkPVRadVizRepresentation
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMultiVariateColorEditorWidget : public pqPropertyGroupWidget
{
  Q_OBJECT
  typedef pqPropertyGroupWidget Superclass;

public:
  pqMultiVariateColorEditorWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = nullptr);
  ~pqMultiVariateColorEditorWidget() override;

private Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Creates (on first use) the RadViz view showing the input to the
   * MultiVariateToRGBA filter and its representation, or, if one already
   * exists (created earlier, or found by findExistingRadViz()), makes it
   * the active view.
   */
  void showRadViz();

  /**
   * Rotates LensCenter around the origin by the change the user made to
   * VerticesRotation, as vtkMultiVariateToRGBA::SetVerticesAndLensRotation
   * does, so the lens keeps the same data points and only the colors change.
   * Called when the VerticesRotation control is edited, before its property
   * link stores the new rotation.
   */
  void rotateLensWithVertices();

  ///@{
  /**
   * Stops tracking the RadViz representation, its ResampleToImage input or
   * its view if the object being removed is one of them, so that this
   * widget does not keep referring to a proxy that is about to be
   * destroyed. The live property sync between the RadViz representation
   * and the MultiVariateToRGBA filter (owned by the RadViz view itself,
   * see pqRadVizChartView) is unaffected: it is torn down independently.
   */
  void sourceRemoved(pqPipelineSource* source);
  void representationRemoved(pqRepresentation* repr);
  void viewRemoved(pqView* view);
  ///@}

private:
  Q_DISABLE_COPY(pqMultiVariateColorEditorWidget)

  /**
   * Looks for a RadViz representation already connected (through
   * RADVIZ_SOURCE_PROPERTY, see pqRadVizChartView) to this widget's
   * MultiVariateToRGBA filter and, if found, restores this widget's
   * tracking of it and its ResampleToImage input and view.
   */
  void findExistingRadViz();

  /**
   * Stops tracking the RadViz representation, its ResampleToImage input
   * and its view. Does not destroy any of them, nor does it affect the
   * connection between them and the MultiVariateToRGBA filter (owned by
   * the RadViz view itself): they are deleted, and disconnected, through
   * the normal ParaView interface.
   */
  void forgetRadViz();

  /**
   * Connects this->Internal->RadVizView's filterPropertiesStagedFromView()
   * signal so that interacting with the RadViz view (rotating vertices,
   * dragging or resizing the lens) makes this widget emit
   * changeAvailable()/changeFinished(), the same way editing its own
   * controls does, so that Auto Apply (see pqApplyBehavior) also picks up
   * changes made that way. Called right after RadVizView is assigned; a
   * no-op if it is null. The connection is torn down, along with
   * everything else this widget tracks about the RadViz view, by
   * forgetRadViz().
   */
  void connectRadVizView();

  /**
   * Renames the RadViz view to its base name (recorded when the view was
   * created or found) followed by the MultiVariateToRGBA filter's current
   * pipeline browser name in parenthesis, e.g. "RadVizChartView1
   * (MultiVariateToRGBA1)". Does nothing if there is no RadViz view.
   */
  void updateRadVizViewAnnotation();

  /**
   * Renames the RadViz view back to its base name, dropping the "(filter
   * name)" annotation. Called when the MultiVariateToRGBA filter this
   * widget is editing is removed, since the view is no longer showing
   * (or letting the user edit) any filter's properties.
   */
  void removeRadVizViewAnnotation();

  /**
   * Shows the visible components of the filter's MultiVariateArray as the
   * series of the newly created RadViz representation, and hides the other
   * series (see pqRadVizChartView::showFilterComponents()). Called once,
   * right after the RadViz representation is created in showRadViz().
   */
  void selectMultiVariateArrayComponents();

  class pqInternal;
  pqInternal* Internal;
};

#endif
