// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Tests pqPartitionEditorWidget, the editor for the PartitionIndices/
 * PartitionOffsets properties of MultiVariateToRGBA and RadVizRepresentation.
 *
 * What it checks:
 * - the list shown for given property values, including the identity
 *   fallback for empty or invalid values, which must not be written back;
 * - which buttons are enabled for each row;
 * - that Move Up/Down and Add/Remove Separator change the list, emit only
 *   the signals for the values that changed, and update the properties;
 * - that a component alone in its group cannot move and the trailing
 *   separator is fixed;
 * - that a change in the components re-renders the list and clears the
 *   partition;
 * - that 2000 random edits always leave a valid partition;
 * - that the Rescale buttons set the CustomRange of the selected group to
 *   its data range or to a range typed in the dialog, also for a composite
 *   input, are disabled with nothing selected or without the data ranges
 *   they need, and are hidden without a CustomRange property;
 * - that the edits keep an element selected, so the Rescale buttons stay
 *   enabled;
 * - and that editing the partition, the array or the visible series
 *   clears CustomRange.
 *
 * How it does it: the test runs a builtin session and loads two small test
 * proxies from inline XML, so it needs no data or real filter. Their
 * "Components" property is an enumeration domain in EnumComponents (like
 * the filter's PartitionNumberOfComponents) and a (name, visible) list in
 * SeriesComponents (like a representation's SeriesVisibility). Each case
 * builds a fresh editor, selects rows and clicks the buttons directly,
 * and compares the list, written as a string such as "0 1 | 2 3 4 |"
 * ('|' is a separator), and the proxy's property values with the expected
 * ones. RangeComponents adds a CustomRange property and a
 * NumberOfComponentsDomain on the Normals of a PlaneSource input, whose
 * component ranges are [0, 0], [0, 0] and [1, 1].
 */

#ifndef PartitionEditorWidget_h
#define PartitionEditorWidget_h

#include "vtkSmartPointer.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class QListWidget;
class QToolButton;
class pqPartitionEditorWidget;
class pqServer;
class vtkSMProxy;

class PartitionEditorWidgetTester : public QObject
{
  Q_OBJECT

public:
  PartitionEditorWidgetTester(pqServer* server);

private Q_SLOTS:
  void initTestCase();
  void cleanup();

  void defaultsToIdentity();
  void labelsUseComponentNames();
  void rendersPartitionFromProperties();
  void invalidPartitionFallsBackToIdentity();
  void buttonStates();
  void moveWithinGroup();
  void moveAcrossSeparator();
  void loneComponentCannotMove();
  void addSeparator();
  void removeSeparator();
  void trailingSeparatorIsFixed();
  void seriesVisibilityChangesComponentCount();
  void randomEditsKeepPartitionValid();
  void rescaleButtonsHiddenWithoutCustomRange();
  void rescaleDisabledWithoutSelection();
  void editsKeepSelection();
  void rescaleToDataRange();
  void rescaleToCustomRange();
  void partitionEditClearsCustomRange();
  void arrayChangeClearsCustomRange();
  void seriesVisibilityChangeClearsCustomRange();
  void visibleComponentsLimitComponents();
  void visibleComponentsFromPythonKeepPartition();
  void naturalOrderPlacesMagnitude();
  void compositeInputRescales();

private:
  /**
   * Creates the \p proxyName test proxy and an editor for it, and finds the
   * editor's list and buttons. Returns false on failure, so call it inside
   * QVERIFY: a QVERIFY in a helper returns only from the helper.
   */
  bool createEditor(const char* proxyName);
  void setPartition(const QList<int>& indices, const QList<int>& offsets);
  QList<int> propertyValues(const char* name) const;
  QList<double> customRange() const;
  QString rows() const;
  QStringList labels() const;
  QString buttonStates(int row) const;
  /**
   * Returns an empty string if \p row is the only selected row and the
   * Rescale buttons are enabled, otherwise a description of the problem.
   */
  QString selectionError(int row) const;
  /**
   * Checks that the list is a valid partition of \p numComponents components:
   * it ends with a separator, does not start with one, has no adjacent
   * separators, and holds every component exactly once. Also checks that
   * the widget's values and the proxy's Indices/Offsets match the list.
   * Returns a description of the first problem found, or an empty string.
   * It returns a message instead of using QVERIFY, so the caller can check it
   * with QVERIFY2: a QVERIFY in a helper returns only from the helper.
   */
  QString partitionError(int numComponents) const;

  pqServer* Server;
  vtkSmartPointer<vtkSMProxy> Proxy;
  vtkSmartPointer<vtkSMProxy> Input;
  QPointer<pqPartitionEditorWidget> Editor;
  QListWidget* List = nullptr;
  QToolButton* Up = nullptr;
  QToolButton* Down = nullptr;
  QToolButton* Add = nullptr;
  QToolButton* Remove = nullptr;
  QToolButton* Rescale = nullptr;
  QToolButton* RescaleCustom = nullptr;
};

#endif
