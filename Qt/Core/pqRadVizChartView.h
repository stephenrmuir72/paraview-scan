// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRadVizChartView_h
#define pqRadVizChartView_h

#include "pqContextView.h"

#include "vtkSmartPointer.h" // for vtkSmartPointer

#include <QPoint>
#include <QPointer>

#include <optional>

class vtkSMSourceProxy;
class vtkEventQtSlotConnect;
class vtkObject;
class pqDataRepresentation;
class pqPipelineSource;
class pqRepresentation;
class pqServerManagerModelItem;
class QEvent;
class QTimer;

/**
 * pqView subclass of RadVizChartView chart view. Besides passing the view
 * type of pqView in the constructor, this class:
 *
 * - Adds a right-click context menu letting the user connect the view to,
 *   or disconnect it from, the `MultiVariateToRGBA` filter it is
 *   recorded as connected to (see RADVIZ_SOURCE_PROPERTY): if the view's
 *   representation is already connected to a filter, the menu offers
 *   "Disconnect from <filter name>"; otherwise, it looks for a
 *   `MultiVariateToRGBA` filter that is a descendant of an ancestor-or-self
 *   of the source currently shown in the view and, if one is found, offers
 *   "Connect to <filter name>", or a disabled "Connect to
 *   MultiVariateToRGBA" if none is found.
 *
 * - Whenever this view's representation is (or becomes) connected to a
 *   MultiVariateToRGBA filter, keeps the synced properties
 *   (VerticesRotation, VerticesClockwiseOrdering, LensCenter, LensRadius,
 *   PartitionIndices, PartitionOffsets and CustomRange) of
 *   the filter and of the representation in sync in both directions:
 *   editing the filter's
 *   properties (e.g. through pqMultiVariateColorEditorWidget) updates the
 *   view right away;
 *   interacting with the view (rotating the vertices, dragging or resizing
 *   the lens) updates the filter's properties and marks it modified,
 *   debounced: whenever the mouse pauses for a short while during the drag,
 *   and at the end of the drag gesture (mouse release) if anything is still
 *   pending. This does not push the change to the filter's VTK object (so a
 *   RenderView showing the filter's output is only updated once the user
 *   clicks Apply). The view itself still tracks the mouse continuously
 *   while dragging; only staging the change onto the filter is debounced
 *   (PartitionIndices, PartitionOffsets and CustomRange are never changed this way --
 *   there is no interactive gesture for them). Changing the
 *   representation's properties any other way (e.g. through its own
 *   Properties/Display panel) updates the filter's properties immediately.
 */
class PQCORE_EXPORT pqRadVizChartView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  static QString chartViewType() { return "RadVizChartView"; }

  pqRadVizChartView(const QString& group, const QString& name, vtkSMContextViewProxy* viewModule,
    pqServer* server, QObject* parent = nullptr);
  ~pqRadVizChartView() override;

  /**
   * Name of the RadViz representation's ProxyProperty that references the
   * `MultiVariateToRGBA` filter it is connected to. Set by
   * pqMultiVariateColorEditorWidget, which also uses it to find an existing
   * RadViz representation. A proxy reference, unlike a global ID, survives
   * reloading a state file.
   */
  static const char* const RADVIZ_SOURCE_PROPERTY;

  /**
   * Disconnects every representation connected (RADVIZ_SOURCE_PROPERTY) to
   * filter. Call it before deleting filter: the property makes the
   * representations consumers of filter, so deleting it would delete them.
   */
  static void disconnectAllFromFilter(pqPipelineSource* filter);

  /**
   * Renames the view to "<name> (<filterName>)", replacing the annotation it
   * added before. An empty filterName removes it. Only the annotation added
   * here is removed, so a name the user ends with parentheses is kept.
   */
  void setFilterAnnotation(const QString& filterName);

  /**
   * Returns the filter name the view's name is annotated with, or an empty
   * string.
   */
  QString filterAnnotation();

  /**
   * Makes the series of the RadViz representation radVizProxy the visible
   * components (see the VisibleComponents property) of the MultiVariateArray
   * of the MultiVariateToRGBA filter filterProxy, and hides every other
   * series. Series names are "<ArrayName>_<ComponentName>" (see
   * vtkSplitColumnComponents, NAMES_WITH_UNDERSCORES). Uses the unchecked
   * (pending Apply) VisibleComponents if unchecked is true.
   */
  static void showFilterComponents(
    vtkSMProxy* radVizProxy, vtkSMProxy* filterProxy, bool unchecked = false);

  /**
   * Returns true while the RadViz view of the representation radVizProxy
   * changes the representation's series itself, as when it shows the
   * connected filter's components, not because of a user edit.
   */
  static bool isUpdatingSeries(vtkSMProxy* radVizProxy);

  /**
   * Temporarily suspends the RadViz representation -> filter sync normally
   * triggered by any change to the representation's synced properties
   * (see syncFilterFromRadViz()), for
   * the duration of \p func(). Used by previewFilter() to push a
   * not-yet-Applied edit onto the representation for live preview without
   * that change being echoed back onto the filter's actual property.
   */
  template <typename Func>
  void withReverseSyncSuspended(Func func)
  {
    // restores the count even if func() throws.
    struct Guard
    {
      int& Count;
      explicit Guard(int& count)
        : Count(count)
      {
        ++this->Count;
      }
      ~Guard() { --this->Count; }
    } guard(this->ReverseSyncSuspendCount);
    func();
  }

Q_SIGNALS:
  /**
   * Emitted at the end of syncFilterFromRadViz(), i.e. whenever interacting
   * with the view (rotating vertices, dragging or resizing the lens)
   * stages the result onto the connected MultiVariateToRGBA filter's
   * pending-Apply (unchecked) properties. pqMultiVariateColorEditorWidget
   * listens for this to emit its own changeAvailable()/changeFinished()
   * signals, the same way editing its controls directly does, so that
   * Auto Apply (see pqApplyBehavior) picks up the change. Not emitted
   * while the reverse sync is suspended (see withReverseSyncSuspended()).
   */
  void filterPropertiesStagedFromView();

  /**
   * Emitted when the view is disconnected from its MultiVariateToRGBA filter
   * through its context menu, so that pqMultiVariateColorEditorWidget stops
   * tracking it.
   */
  void disconnectedFromFilter();

public Q_SLOTS:
  /**
   * (Re)synchronizes the live property links between this view's visible
   * representation and the MultiVariateToRGBA filter it is recorded
   * (RADVIZ_SOURCE_PROPERTY) as connected to, replacing any previously
   * established links. Tears the links down (without error) if there is
   * no such representation/filter pair anymore. When the link changes, adds
   * the filter's name to the view's name, or removes it. Safe, and cheap, to call
   * any time the connection may have changed, e.g. after setting or
   * clearing RADVIZ_SOURCE_PROPERTY by hand.
   */
  void updateConnection();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Captures right-clicks on the view's widget (installed as an event
   * filter on it by pqContextView::createWidget()) to pop up the connect/
   * disconnect context menu.
   */
  bool eventFilter(QObject* caller, QEvent* e) override;

private Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Clears the RADVIZ_SOURCE_PROPERTY on the RadViz representation's
   * proxy, disconnecting the view from the MultiVariateToRGBA filter it
   * was connected to, and drops the "(filter name)" suffix from the
   * view's name.
   */
  void disconnectFromFilter();

  /**
   * Sets the RADVIZ_SOURCE_PROPERTY on the RadViz representation's proxy
   * to filter, one of findConnectableFilters(), connecting the view to it,
   * and appends "(filter name)" to the view's name. As with Show RadViz,
   * the view takes the filter's pending (unchecked) values and visible
   * components, replacing its own.
   */
  void connectToFilter(pqPipelineSource* filter);

  /**
   * Watches the RADVIZ_SOURCE_PROPERTY of the representation just added
   * and calls updateConnection(), in case it is already recorded as
   * connected to an existing MultiVariateToRGBA filter (e.g. restored from
   * a saved state).
   */
  void onRepresentationAdded(pqRepresentation* repr);

  /**
   * Calls updateConnection(), so the link and the view's name follow the
   * visible representation.
   */
  void onRepresentationVisibilityChanged(pqRepresentation* repr, bool visible);

  /**
   * Stops watching the RADVIZ_SOURCE_PROPERTY of the representation just
   * removed, and tears down the live property links if they were
   * established for it.
   */
  void onRepresentationRemoved(pqRepresentation* repr);

  /**
   * Calls updateConnection(), in case the source just added is the
   * MultiVariateToRGBA filter this view's representation is already
   * recorded as connected to (e.g. restored from a saved state, where the
   * filter's proxy may be registered after the representation's).
   */
  void onSourceAdded(pqPipelineSource* source);

  /**
   * Tears down the live property links if the source just removed is the
   * MultiVariateToRGBA filter they were established for, and drops the
   * "(filter name)" suffix from the view's name if it names that source.
   */
  void onSourceRemoved(pqPipelineSource* source);

  /**
   * Updates the "(filter name)" suffix of the view's name when the connected
   * MultiVariateToRGBA filter is renamed.
   */
  void onNameChanged(pqServerManagerModelItem* item);

  /**
   * Queues updateConnection() when a representation's RADVIZ_SOURCE_PROPERTY
   * changes (undo, Python), so callers that set it can finish their setup first.
   */
  void onSourcePropertyModified();

  /**
   * Called when a synced property of the filter changes (Apply, Python, undo,
   * state): copies it onto the representation's same-named property, with
   * the reverse sync suspended, live-updating the view.
   */
  void syncRadVizFromFilter(vtkObject* filterProperty);

  /**
   * Called when any synced property of the RadViz representation changes.
   * Stages the change onto the filter right away (see
   * stageFilterFromRadViz()). A no-op while the reverse sync is suspended
   * or while InteractionInProgress is true (drags are handled by
   * onRadVizInteraction() instead).
   */
  void syncFilterFromRadViz();

  /**
   * Called on every change during a drag of the vertices or lens (the
   * representation proxy's InteractionEvent): (re)starts
   * InteractionSyncTimer, so the change is staged only once the mouse
   * pauses.
   */
  void onRadVizInteraction();

  /**
   * Called when InteractionSyncTimer fires mid-drag: updates the
   * representation proxy's properties from its VTK object, then stages
   * them onto the filter.
   */
  void onInteractionSyncTimeout();

  /**
   * Copies the RadViz representation's synced properties onto the
   * connected MultiVariateToRGBA filter's same-named properties, without
   * pushing them to the filter's VTK object, and marks the filter modified
   * so that Apply gets highlighted. Cancels any pending debounced sync.
   */
  void stageFilterFromRadViz();

  /**
   * Shows the filter's VisibleComponents as the representation's series (see
   * showFilterComponents()), the unchecked (pending Apply) ones if unchecked
   * is true. Called when the link is established, when the filter's output
   * is updated and when the series would leave too few components visible;
   * later changes reach the series through previewFilter().
   */
  void syncSeriesFromFilter(bool unchecked = false);

  /**
   * Called when the filter's output is updated: shows its unchecked
   * VisibleComponents as the series. Before the filter's first Apply its
   * output has no MultiVariateArray, so the series could not be set earlier.
   */
  void onFilterDataUpdated();

  /**
   * Called when the filter's unchecked (pending Apply) synced properties or
   * VisibleComponents change, e.g. in its panel or on Reset: previews them on
   * the representation without staging them back onto the filter.
   */
  void previewFilter();

  /**
   * Called when the representation's SeriesVisibility changes, e.g. in its
   * Series Parameters: hides the series that are not components of the
   * filter's MultiVariateArray and, if the visible components changed,
   * stages them onto the filter and clears the partition and CustomRange,
   * which refer to the visible components.
   */
  void syncFilterFromSeries();

  ///@{
  /**
   * Tracks whether an interactive drag of the vertices or lens is
   * currently in progress (see InteractionInProgress)
   */
  void onRadVizStartInteraction();
  void onRadVizEndInteraction();
  ///@}

private:
  Q_DISABLE_COPY(pqRadVizChartView)

  /**
   * Pops up the connect/disconnect context menu at \p globalPos.
   */
  void showContextMenu(const QPoint& globalPos);

  /**
   * Returns the RadViz representation currently shown in this view: the
   * first visible pqDataRepresentation among this view's representations,
   * or nullptr if none is visible.
   */
  pqDataRepresentation* radVizRepresentation() const;

  /**
   * Returns the MultiVariateToRGBA filter \p repr's proxy is recorded
   * (through RADVIZ_SOURCE_PROPERTY) as connected to, or nullptr if \p
   * repr is null, the property is unset, or the recorded filter's
   * pqPipelineSource wrapper does not exist (yet).
   */
  pqPipelineSource* connectedFilter(pqDataRepresentation* repr) const;

  /**
   * Returns the MultiVariateToRGBA filters that see the same data as \p
   * repr: for S -> [ResampleToImage ->] repr's input, the filters in
   * S -> [ResampleToImage ->] MultiVariateToRGBA. Each is listed in the
   * view's context menu.
   */
  QList<pqPipelineSource*> findConnectableFilters(pqDataRepresentation* repr) const;

  /**
   * Establishes the live property links between \p repr and \p filter,
   * replacing any previously established ones, and records them as the
   * currently linked representation/filter. The view starts with the
   * filter's pending (unchecked) values, so edits not yet applied are kept.
   */
  void establishLink(pqDataRepresentation* repr, pqPipelineSource* filter);

  /**
   * Releases the live property links, if any, and forgets the currently
   * linked representation/filter.
   */
  void teardownLink();

  /**
   * Where the right button was pressed, unset when it is not pressed.
   * Used for the right-click menu.
   */
  std::optional<QPoint> Position;

  // The representation/filter pair the live property links currently
  // connect, or null if none. Kept so onRepresentationRemoved() and
  // onSourceRemoved() can tell whether the representation/source being
  // removed is the one the links reference, and updateConnection() can
  // tell whether the connection actually changed.
  QPointer<pqDataRepresentation> LinkedRepresentation;
  QPointer<pqPipelineSource> LinkedFilter;

  // Watches, while the view is linked to the filter:
  // - the filter's synced properties (see syncRadVizFromFilter());
  // - the filter's pending synced properties and VisibleComponents (see
  //   previewFilter());
  // - the filter's data updates (see onFilterDataUpdated());
  // - the representation's synced properties (see syncFilterFromRadViz());
  // - the representation's SeriesVisibility (see syncFilterFromSeries());
  // - the representation proxy's drag events, vertices rotation or lens
  //   drag/resize (see onRadVizStartInteraction(), onRadVizInteraction() and
  //   onRadVizEndInteraction()).
  vtkSmartPointer<vtkEventQtSlotConnect> FilterConnector;

  // Watches the RADVIZ_SOURCE_PROPERTY of the view's representations (see
  // onSourcePropertyModified()).
  vtkSmartPointer<vtkEventQtSlotConnect> SourcePropertyConnector;

  // Single-shot debounce timer, active while a drag has changes not yet
  // staged onto the filter (see onRadVizInteraction()).
  QTimer* InteractionSyncTimer;

  // True between onRadVizStartInteraction() and onRadVizEndInteraction(),
  // i.e. while an interactive drag of the vertices or lens is in progress.
  bool InteractionInProgress = false;

  // Non-zero while withReverseSyncSuspended()'s func() is running: makes
  // syncFilterFromRadViz() a no-op.
  int ReverseSyncSuspendCount = 0;

  // True while stageFilterFromRadViz() copies the synced properties to the
  // filter one at a time: makes previewFilter() a no-op, as it would copy the
  // filter's properties not yet staged (e.g. the old LensCenter after a new
  // VerticesRotation) back onto the representation.
  bool StagingFilter = false;

  // True while syncSeriesFromFilter(), previewFilter() or syncFilterFromSeries()
  // changes the representation's SeriesVisibility: makes syncFilterFromSeries() a
  // no-op (see also isUpdatingSeries()).
  bool UpdatingSeriesVisibility = false;
};

#endif
