// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPartitionEditorWidget_h
#define pqPartitionEditorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QVariant>

#include <array>
#include <vector>

class QListWidget;
class QListWidgetItem;
class QToolButton;
class vtkEventQtSlotConnect;
class vtkSMProperty;
class vtkSMPropertyGroup;
class vtkSMStringVectorProperty;

/**
 * pqPartitionEditorWidget is the custom property-group widget for a
 * `PartitionIndices`/`PartitionOffsets` property pair (see
 * vtkMultiVariateToRGBA::SetPartitionIndices for their layout), used
 * by both the `MultiVariateToRGBA` filter and the
 * `RadVizRepresentation` proxy. It shows the components being
 * processed as an ordered list, 0 to N - 1, with "separator" rows
 * splitting the list into partition groups: everything between two
 * separators (or between the start of the list and the first
 * separator) forms one group.
 *
 * The property group must additionally provide a third, "Components"
 * function pointing to a property this widget can use to determine N and,
 * if available, component names (see numberOfComponents() and
 * componentLabel()): either a property carrying a NumberOfComponentsDomain
 * (N = domain entry count, as used by the filter's hidden
 * PartitionNumberOfComponents property), or a (name, visible) pairs
 * StringVectorProperty such as a representation's SeriesVisibility (N =
 * number of entries currently marked visible). In the latter case, note
 * that PartitionIndices/PartitionOffsets are interpreted in the natural
 * (unsorted) order of the proxy's input columns/arrays, not in
 * SeriesVisibility's own (alphabetical) order -- see
 * naturalOrderVisibleNames(), which this widget uses instead whenever it
 * needs to turn an index into a name or vice versa. With a
 * NumberOfComponentsDomain, an optional "VisibleComponents" function
 * (see vtkMultiVariateToRGBA::AddVisibleVariableIndex) limits the components
 * to the visible ones: N is then their number, and index i is the i-th
 * visible component.
 *
 * - Selecting a component and clicking "Move Up"/"Move Down" moves it one
 *   position, letting it cross a separator to join the group before/after
 *   it.
 * - "Add Separator" inserts a new separator right after the selected
 *   component, creating a new group boundary there.
 * - Selecting a separator and clicking "Remove Separator" removes it,
 *   merging the two groups on either side.
 * - "Rescale to data range" and "Rescale to custom range" set the
 *   CustomRange (see vtkMultiVariateToRGBA::SetCustomRange) of the group
 *   of the selected element (a component, or the separator closing the
 *   group) to the data range of its components or to a range entered by
 *   the user. When CustomRange is empty, the other groups get their data
 *   range. These buttons are shown only if the property group provides a
 *   "CustomRange" function. Editing the partition clears CustomRange.
 *
 * A separator can never be inserted before the first component (that
 * would create an empty leading group), and the list always ends with a
 * separator, which can neither be moved nor removed, so that the last
 * group always closes at the last component. This guarantees at least 1
 * (that one) and at most N (one after every component) separators
 * overall.
 *
 * The list is (re)initialized to identity order (0, 1, ..., N - 1) with a
 * single trailing separator -- the unpartitioned default -- whenever
 * PartitionIndices/PartitionOffsets are empty, or no longer describe a
 * valid partition of the current number of components N (e.g. because
 * the selected array, or its number of components, changed). This
 * fallback is display-only: it is not pushed onto the properties until
 * the user actually interacts with the widget. The exception is when the
 * components change (see componentsChanged()): the partition and
 * CustomRange are then cleared, since they would refer to different
 * components.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPartitionEditorWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> partitionIndices READ partitionIndices WRITE setPartitionIndices NOTIFY
      partitionIndicesChanged)
  Q_PROPERTY(QList<QVariant> partitionOffsets READ partitionOffsets WRITE setPartitionOffsets NOTIFY
      partitionOffsetsChanged)
  Q_PROPERTY(
    QList<QVariant> customRange READ customRange WRITE setCustomRange NOTIFY customRangeChanged)
  typedef pqPropertyWidget Superclass;

public:
  pqPartitionEditorWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = nullptr);
  ~pqPartitionEditorWidget() override;

  ///@{
  /**
   * Get/set the component indices, in their current order (skipping
   * separator rows) -- the value pushed to/read from the PartitionIndices
   * property.
   */
  QList<QVariant> partitionIndices() const;
  void setPartitionIndices(const QList<QVariant>& values);
  ///@}

  ///@{
  /**
   * Get/set the separator offsets -- the value pushed to/read from the
   * PartitionOffsets property.
   */
  QList<QVariant> partitionOffsets() const;
  void setPartitionOffsets(const QList<QVariant>& values);
  ///@}

  ///@{
  /**
   * Get/set the [min, max] range of each partition group -- the value
   * pushed to/read from the CustomRange property.
   */
  QList<QVariant> customRange() const;
  void setCustomRange(const QList<QVariant>& values);
  ///@}

Q_SIGNALS:
  void partitionIndicesChanged();
  void partitionOffsetsChanged();
  void customRangeChanged();

private Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  ///@{
  /**
   * Moves the selected component up/down by one position, possibly
   * crossing a separator into the neighboring group. A no-op (and
   * disabled, see updateButtonStates()) if the current selection/position
   * makes the move unsafe (see the class documentation).
   */
  void moveUp();
  void moveDown();
  ///@}

  /**
   * Inserts a new separator right after the selected component. A no-op
   * (and disabled) if the selection is not a component, already has a
   * separator right after it, or the maximum number of separators (the
   * number of components) has been reached.
   */
  void addSeparator();

  /**
   * Removes the selected separator. A no-op (and disabled) if the
   * selection is not a separator, or is the fixed trailing one.
   */
  void removeSeparator();

  ///@{
  /**
   * Sets the custom range of the selected group to the data range of its
   * components, or to a range entered by the user in a dialog. A no-op
   * (and disabled) if no element is selected.
   */
  void rescaleToDataRange();
  void rescaleToCustomRange();
  ///@}

  /**
   * Updates the Move Up/Down/Add/Remove Separator and Rescale buttons' enabled state
   * based on the current selection and list arrangement.
   */
  void updateButtonStates();

  /**
   * Called when the "Components" property (see the class documentation)
   * or its domain changes, i.e. when the number of components N may have
   * changed. Re-renders the list, falling back to identity order if the
   * current PartitionIndices/PartitionOffsets no longer match the new N.
   * If the components changed (see componentsKey()), clears
   * PartitionIndices/PartitionOffsets and CustomRange. Nothing is cleared
   * when there were no components before, as when loading a state, or on a
   * filter when neither VisibleComponents nor the array has a pending value,
   * as when they are set from Python.
   */
  void componentsChanged();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqPartitionEditorWidget)

  /**
   * (Re)populates ListWidget from CachedIndices/CachedOffsets, falling
   * back to identity order (0, 1, ..., N - 1) with a single trailing
   * separator if they are empty or inconsistent with the current number
   * of components (see numberOfComponents()). Never modifies
   * CachedIndices/CachedOffsets: the fallback, when used, is display-only.
   */
  void renderList();

  /**
   * Recomputes CachedIndices/CachedOffsets from ListWidget's current
   * arrangement (the result of a user-driven edit) and emits
   * partitionIndicesChanged()/partitionOffsetsChanged() for values that
   * changed, so the property links push them onto the
   * PartitionIndices/PartitionOffsets properties.
   */
  void commitList();

  /**
   * Returns the number of components N currently being processed, from
   * ComponentsProperty (see the class documentation), or 0 if not
   * available (e.g. no array selected yet, on the filter, or every series
   * hidden, on the representation).
   */
  int numberOfComponents() const;

  /**
   * Returns the visible components (see VisibleComponentsProperty), as
   * indices in [0, total) in increasing order, where total is the number of
   * components of the array: indices outside that range are dropped, and
   * all of them are returned when there is no such property or it is empty,
   * as vtkMultiVariateToRGBA processes all components for an empty list (see
   * vtkMultiVariateToRGBA::AddVisibleVariableIndex).
   */
  std::vector<int> visibleComponentIndices(int total) const;

  /**
   * Returns a display label for component \p value (e.g. "2: Pressure"
   * when ComponentsProperty has a name for it, or plain "2" otherwise).
   */
  QString componentLabel(int value) const;

  /**
   * Returns the names of \p svp's currently visible entries (see
   * visibleNames() in the .cxx), reordered to match the natural
   * (unsorted) order of the proxy's input columns/arrays -- the order
   * PartitionIndices/PartitionOffsets are actually interpreted in (see
   * vtkMultiVariateToRGBA::GetVisibleVariables), as opposed to
   * SeriesVisibility's own alphabetical order (see
   * vtkSMChartSeriesSelectionDomain). If \p ranges is not null, it is set
   * to the data range of each returned name.
   */
  QStringList naturalOrderVisibleNames(
    vtkSMStringVectorProperty* svp, std::vector<std::array<double, 2>>* ranges = nullptr) const;

  /**
   * Returns the data range of each component, indexed by component
   * value, or an empty vector if not available. A range with min > max
   * means the range of that component is not known.
   */
  std::vector<std::array<double, 2>> componentRanges() const;

  /**
   * The array property the "Components" property's domain depends on
   * (ArraySelection), or nullptr.
   */
  vtkSMProperty* arrayProperty() const;

  /**
   * Identifies the components being processed: the selected array name
   * and the component labels. Empty if there are no components.
   */
  QStringList componentsKey() const;

  /**
   * For a (name, visible) "Components" property, such as SeriesVisibility,
   * returns true if a series listed before changed visibility since the
   * last call. Series added since then, as the series domain does for new
   * arrays, are not edits. Returns true for other "Components" properties.
   */
  bool seriesEdited();

  /**
   * Returns the group of the selected row, or -1 if nothing is selected.
   */
  int selectedGroup() const;

  /**
   * Computes the data range of \p group in \p range. Returns false if it is
   * not available.
   */
  bool groupDataRange(int group, double range[2]) const;

  /**
   * Sets the custom range of \p group to \p range and emits
   * customRangeChanged(). If CustomRange does not have one range per
   * group, the other groups are set to their data range.
   */
  void setGroupCustomRange(int group, const double range[2]);

  /**
   * Returns the number of separator rows currently in ListWidget.
   */
  int separatorCount() const;

  /**
   * Sets ListWidget's height to fit its rows (it scrolls past
   * MAX_VISIBLE_ROWS, see the .cxx), but no shorter than the buttons next
   * to it.
   */
  void updateListHeight();

  ///@{
  /**
   * Returns true if moving the component at \p row up/down by one
   * position is currently allowed (see the class documentation for the
   * constraints).
   */
  bool canMoveUp(int row) const;
  bool canMoveDown(int row) const;
  ///@}

  ///@{
  /**
   * Returns true if inserting/removing a separator at \p row is currently
   * allowed.
   */
  bool canAddSeparatorAfter(int row) const;
  bool canRemoveSeparatorAt(int row) const;
  ///@}

  QListWidget* ListWidget;
  QToolButton* UpButton;
  QToolButton* DownButton;
  QToolButton* AddSeparatorButton;
  QToolButton* RemoveSeparatorButton;
  QToolButton* RescaleButton;
  QToolButton* RescaleCustomButton;
  int ButtonsHeight = 0;
  QList<QVariant> CachedIndices;
  QList<QVariant> CachedOffsets;
  QList<QVariant> CachedCustomRange;
  QStringList ComponentsKey;
  QSet<QString> ListedSeries;
  QSet<QString> ListedVisibleSeries;
  vtkSMProperty* ComponentsProperty = nullptr;
  vtkSMProperty* VisibleComponentsProperty = nullptr;
  vtkEventQtSlotConnect* VTKConnector;
};

#endif
