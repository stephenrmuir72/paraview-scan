// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "PartitionEditorWidget.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPartitionEditorWidget.h"
#include "pqRescaleScalarRangeToCustomDialog.h"
#include "pqServer.h"
#include "pqServerResource.h"
#include "vtkDataObject.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QApplication>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QToolButton>

#include <algorithm>
#include <cstring>
#include <random>
#include <vector>

namespace
{
// Stand-ins for the two proxies that use pqPartitionEditorWidget. They differ
// in the property mapped to the "Components" function, which gives the
// number of components and their names:
// - EnumComponents emulates the MultiVariateToRGBA filter, whose
//   PartitionNumberOfComponents has a NumberOfComponentsDomain (a
//   vtkSMEnumerationDomain with one entry per component of the selected
//   array). A plain EnumerationDomain with 5 fixed entries takes the same
//   code path without needing an input.
// - VisibleEnumComponents adds the filter's VisibleComponents to
//   EnumComponents: only the visible components are partitioned.
// - SeriesComponents emulates RadVizRepresentation, whose SeriesVisibility is
//   a list of (name, visible) pairs; the visible entries are the components.
// Neither proxy has Input or FieldAssociation, so the editor uses
// SeriesVisibility order: naturalOrderPlacesMagnitude tests the reordering to
// the input's array order (naturalOrderVisibleNames) with a real
// RadVizRepresentation.
// - RangeComponents emulates the filter more closely, with a
//   NumberOfComponentsDomain on a real input, and adds CustomRange, to test
//   the Rescale buttons.
const char* const TestProxiesXML = R"(
<ServerManagerConfiguration>
  <ProxyGroup name="partition_editor_test">
    <Proxy name="EnumComponents">
      <IntVectorProperty name="Indices" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <IntVectorProperty name="Offsets" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <IntVectorProperty name="Components" number_of_elements="1" default_values="0">
        <EnumerationDomain name="enum">
          <Entry text="Red" value="0" />
          <Entry text="Green" value="1" />
          <Entry text="Blue" value="2" />
          <Entry text="Alpha" value="3" />
          <Entry text="Extra" value="4" />
        </EnumerationDomain>
      </IntVectorProperty>
      <PropertyGroup label="Partitions">
        <Property name="Indices" function="Indices" />
        <Property name="Offsets" function="Offsets" />
        <Property name="Components" function="Components" />
      </PropertyGroup>
    </Proxy>
    <Proxy name="VisibleEnumComponents">
      <IntVectorProperty name="Indices" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <IntVectorProperty name="Offsets" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <IntVectorProperty name="VisibleComponents" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <IntVectorProperty name="Components" number_of_elements="1" default_values="0">
        <EnumerationDomain name="enum">
          <Entry text="Red" value="0" />
          <Entry text="Green" value="1" />
          <Entry text="Blue" value="2" />
          <Entry text="Alpha" value="3" />
          <Entry text="Extra" value="4" />
        </EnumerationDomain>
      </IntVectorProperty>
      <PropertyGroup label="Partitions">
        <Property name="Indices" function="Indices" />
        <Property name="Offsets" function="Offsets" />
        <Property name="Components" function="Components" />
        <Property name="VisibleComponents" function="VisibleComponents" />
      </PropertyGroup>
    </Proxy>
    <Proxy name="SeriesComponents">
      <IntVectorProperty name="Indices" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <IntVectorProperty name="Offsets" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <StringVectorProperty name="Components" number_of_elements="0" repeat_command="1"
                            number_of_elements_per_command="2" element_types="2 0" />
      <DoubleVectorProperty name="CustomRange" number_of_elements="0" repeat_command="1"
                            number_of_elements_per_command="2" />
      <PropertyGroup label="Partitions">
        <Property name="Indices" function="Indices" />
        <Property name="Offsets" function="Offsets" />
        <Property name="CustomRange" function="CustomRange" />
        <Property name="Components" function="Components" />
      </PropertyGroup>
    </Proxy>
    <Proxy name="RangeComponents">
      <InputProperty name="Input">
        <InputArrayDomain name="input_array" number_of_components="3" />
      </InputProperty>
      <StringVectorProperty name="Array" number_of_elements="5" element_types="0 0 0 0 2"
                            default_values="0" />
      <IntVectorProperty name="Indices" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <IntVectorProperty name="Offsets" number_of_elements="0" repeat_command="1"
                         number_of_elements_per_command="1" />
      <DoubleVectorProperty name="CustomRange" number_of_elements="0" repeat_command="1"
                            number_of_elements_per_command="2" />
      <IntVectorProperty name="Components" number_of_elements="1" default_values="0">
        <NumberOfComponentsDomain name="comps">
          <RequiredProperties>
            <Property function="Input" name="Input" />
            <Property function="ArraySelection" name="Array" />
          </RequiredProperties>
        </NumberOfComponentsDomain>
      </IntVectorProperty>
      <PropertyGroup label="Partitions">
        <Property name="Indices" function="Indices" />
        <Property name="Offsets" function="Offsets" />
        <Property name="CustomRange" function="CustomRange" />
        <Property name="Components" function="Components" />
      </PropertyGroup>
    </Proxy>
  </ProxyGroup>
</ServerManagerConfiguration>
)";

const int SEPARATOR_VALUE = -1;
const int NUMBER_OF_ENUM_COMPONENTS = 5;

QToolButton* findButton(QWidget* parent, const QString& textOrTooltip)
{
  for (QToolButton* button : parent->findChildren<QToolButton*>())
  {
    if (button->text() == textOrTooltip || button->toolTip().startsWith(textOrTooltip))
    {
      return button;
    }
  }
  return nullptr;
}

// Returns the "Partitions" property group of proxy, or nullptr.
vtkSMPropertyGroup* partitionsGroup(vtkSMProxy* proxy)
{
  for (size_t i = 0; i < proxy->GetNumberOfPropertyGroups(); ++i)
  {
    vtkSMPropertyGroup* candidate = proxy->GetPropertyGroup(i);
    if (candidate->GetXMLLabel() && strcmp(candidate->GetXMLLabel(), "Partitions") == 0)
    {
      return candidate;
    }
  }
  return nullptr;
}

QList<int> toIntList(const QList<QVariant>& values)
{
  QList<int> result;
  for (const QVariant& v : values)
  {
    result.append(v.toInt());
  }
  return result;
}
}

//-----------------------------------------------------------------------------
PartitionEditorWidgetTester::PartitionEditorWidgetTester(pqServer* server)
  : Server(server)
{
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::initTestCase()
{
  QVERIFY(this->Server);
  vtkSMSessionProxyManager* pxm = this->Server->proxyManager();
  QVERIFY(pxm->GetProxyDefinitionManager()->LoadConfigurationXMLFromString(::TestProxiesXML));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::cleanup()
{
  delete this->Editor;
  this->Proxy = nullptr;
  this->Input = nullptr;
}

//-----------------------------------------------------------------------------
bool PartitionEditorWidgetTester::createEditor(const char* proxyName)
{
  vtkSMSessionProxyManager* pxm = this->Server->proxyManager();
  this->Proxy.TakeReference(pxm->NewProxy("partition_editor_test", proxyName));
  if (!this->Proxy || this->Proxy->GetNumberOfPropertyGroups() != 1)
  {
    return false;
  }
  if (strcmp(proxyName, "RangeComponents") == 0)
  {
    // a plane's normals are all (0, 0, 1).
    this->Input.TakeReference(pxm->NewProxy("sources", "PlaneSource"));
    auto* source = vtkSMSourceProxy::SafeDownCast(this->Input);
    if (!source)
    {
      return false;
    }
    source->UpdateVTKObjects();
    source->UpdatePipeline();
    vtkSMPropertyHelper(this->Proxy, "Input").Set(this->Input);
    // set the array name, element 4 of [idx, port, connection, association, name],
    // before creating the editor, so it starts with Normals' 3 components
    vtkSMPropertyHelper(this->Proxy, "Array").Set(4, "Normals");
  }
  this->Editor = new pqPartitionEditorWidget(this->Proxy, this->Proxy->GetPropertyGroup(0));
  this->List = this->Editor->findChild<QListWidget*>();
  this->Up = ::findButton(this->Editor, "Move the selected component up");
  this->Down = ::findButton(this->Editor, "Move the selected component down");
  this->Add = ::findButton(this->Editor, "Add Separator");
  this->Remove = ::findButton(this->Editor, "Remove Separator");
  this->Rescale = ::findButton(this->Editor, "Rescale the group of the selected element to data");
  this->RescaleCustom =
    ::findButton(this->Editor, "Rescale the group of the selected element to custom");
  return this->List && this->Up && this->Down && this->Add && this->Remove && this->Rescale &&
    this->RescaleCustom;
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::setPartition(const QList<int>& indices, const QList<int>& offsets)
{
  const std::vector<int> i(indices.begin(), indices.end());
  const std::vector<int> o(offsets.begin(), offsets.end());
  vtkSMPropertyHelper(this->Proxy, "Indices").Set(i.data(), static_cast<unsigned int>(i.size()));
  vtkSMPropertyHelper(this->Proxy, "Offsets").Set(o.data(), static_cast<unsigned int>(o.size()));
}

//-----------------------------------------------------------------------------
QList<int> PartitionEditorWidgetTester::propertyValues(const char* name) const
{
  QList<int> result;
  for (int v : vtkSMUncheckedPropertyHelper(this->Proxy, name).GetIntArray())
  {
    result.append(v);
  }
  return result;
}

//-----------------------------------------------------------------------------
QList<double> PartitionEditorWidgetTester::customRange() const
{
  QList<double> result;
  for (double v : vtkSMUncheckedPropertyHelper(this->Proxy, "CustomRange").GetDoubleArray())
  {
    result.append(v);
  }
  return result;
}

//-----------------------------------------------------------------------------
QString PartitionEditorWidgetTester::rows() const
{
  QStringList result;
  for (int row = 0; row < this->List->count(); ++row)
  {
    const int value = this->List->item(row)->data(Qt::UserRole).toInt();
    result.append(value == ::SEPARATOR_VALUE ? QString("|") : QString::number(value));
  }
  return result.join(' ');
}

//-----------------------------------------------------------------------------
QStringList PartitionEditorWidgetTester::labels() const
{
  QStringList result;
  for (int row = 0; row < this->List->count(); ++row)
  {
    if (this->List->item(row)->data(Qt::UserRole).toInt() != ::SEPARATOR_VALUE)
    {
      result.append(this->List->item(row)->text());
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
QString PartitionEditorWidgetTester::buttonStates(int row) const
{
  this->List->setCurrentRow(row);
  QString result;
  result += this->Up->isEnabled() ? 'U' : '-';
  result += this->Down->isEnabled() ? 'D' : '-';
  result += this->Add->isEnabled() ? 'A' : '-';
  result += this->Remove->isEnabled() ? 'R' : '-';
  return result;
}

//-----------------------------------------------------------------------------
QString PartitionEditorWidgetTester::selectionError(int row) const
{
  const QList<QListWidgetItem*> selected = this->List->selectedItems();
  if (selected.size() != 1 || selected[0] != this->List->item(row))
  {
    return QString("row %1 is not the only selected row in %2").arg(row).arg(this->rows());
  }
  if (!this->Rescale->isEnabled() || !this->RescaleCustom->isEnabled())
  {
    return "Rescale buttons are disabled";
  }
  return QString();
}

//-----------------------------------------------------------------------------
QString PartitionEditorWidgetTester::partitionError(int numComponents) const
{
  const int count = this->List->count();
  auto isSeparator = [this](int row)
  { return this->List->item(row)->data(Qt::UserRole).toInt() == ::SEPARATOR_VALUE; };
  if (count == 0 || !isSeparator(count - 1))
  {
    return "missing trailing separator";
  }
  if (isSeparator(0))
  {
    return "leading separator";
  }
  QList<int> indices;
  QList<int> offsets;
  for (int row = 0; row < count; ++row)
  {
    if (isSeparator(row))
    {
      if (isSeparator(row - 1))
      {
        return QString("adjacent separators at row %1").arg(row);
      }
      offsets.append(indices.size());
    }
    else
    {
      indices.append(this->List->item(row)->data(Qt::UserRole).toInt());
    }
  }
  QList<int> sorted = indices;
  std::sort(sorted.begin(), sorted.end());
  for (int i = 0; i < numComponents; ++i)
  {
    if (sorted.size() != numComponents || sorted[i] != i)
    {
      return "components are not a permutation";
    }
  }
  if (::toIntList(this->Editor->partitionIndices()) != indices ||
    ::toIntList(this->Editor->partitionOffsets()) != offsets)
  {
    return "widget values do not match the list";
  }
  if (this->propertyValues("Indices") != indices || this->propertyValues("Offsets") != offsets)
  {
    return "property values do not match the list";
  }
  return QString();
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::defaultsToIdentity()
{
  QVERIFY(this->createEditor("EnumComponents"));
  QCOMPARE(this->rows(), QString("0 1 2 3 4 |"));
  // the identity fallback is display-only.
  QVERIFY(this->propertyValues("Indices").isEmpty());
  QVERIFY(this->propertyValues("Offsets").isEmpty());
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::labelsUseComponentNames()
{
  QVERIFY(this->createEditor("EnumComponents"));
  this->setPartition({ 4, 1, 0, 2, 3 }, { 5 });
  QCOMPARE(
    this->labels(), QStringList({ "4: Extra", "1: Green", "0: Red", "2: Blue", "3: Alpha" }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::rendersPartitionFromProperties()
{
  QVERIFY(this->createEditor("EnumComponents"));
  QSignalSpy changeSpy(this->Editor.data(), &pqPropertyWidget::changeAvailable);
  this->setPartition({ 2, 0, 4, 3, 1 }, { 2, 5 });
  QCOMPARE(this->rows(), QString("2 0 | 4 3 1 |"));
  // a programmatic update is not a user edit, so it must not highlight Apply.
  QCOMPARE(changeSpy.count(), 0);
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::invalidPartitionFallsBackToIdentity()
{
  QVERIFY(this->createEditor("EnumComponents"));
  this->setPartition({ 0, 1 }, { 2 });
  QCOMPARE(this->rows(), QString("0 1 2 3 4 |"));
  this->setPartition({ 0, 1, 1, 3, 4 }, { 5 });
  QCOMPARE(this->rows(), QString("0 1 2 3 4 |"));
  this->setPartition({ 0, 1, 2, 3, 4 }, { 3, 2, 5 });
  QCOMPARE(this->rows(), QString("0 1 2 3 4 |"));
  QCOMPARE(this->propertyValues("Offsets"), QList<int>({ 3, 2, 5 }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::buttonStates()
{
  QVERIFY(this->createEditor("EnumComponents"));
  this->setPartition({ 0, 1, 2, 3, 4 }, { 2, 5 });
  QCOMPARE(this->rows(), QString("0 1 | 2 3 4 |"));
  // select each row; U, D, A, R: Up, Down, Add, Remove enabled, '-' disabled.
  QCOMPARE(this->buttonStates(0), QString("-DA-"));
  QCOMPARE(this->buttonStates(1), QString("UD--"));
  QCOMPARE(this->buttonStates(2), QString("---R"));
  QCOMPARE(this->buttonStates(3), QString("UDA-"));
  QCOMPARE(this->buttonStates(4), QString("UDA-"));
  QCOMPARE(this->buttonStates(5), QString("U---"));
  QCOMPARE(this->buttonStates(6), QString("----"));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::moveWithinGroup()
{
  QVERIFY(this->createEditor("EnumComponents"));
  this->setPartition({ 0, 1, 2, 3, 4 }, { 5 });
  QSignalSpy indicesSpy(this->Editor.data(), &pqPartitionEditorWidget::partitionIndicesChanged);
  QSignalSpy offsetsSpy(this->Editor.data(), &pqPartitionEditorWidget::partitionOffsetsChanged);
  QSignalSpy changeSpy(this->Editor.data(), &pqPropertyWidget::changeAvailable);

  this->List->setCurrentRow(3);
  this->Up->click();
  QCOMPARE(this->rows(), QString("0 1 3 2 4 |"));
  QCOMPARE(this->List->currentRow(), 2);
  QCOMPARE(indicesSpy.count(), 1);
  QCOMPARE(offsetsSpy.count(), 0);
  QVERIFY(changeSpy.count() > 0);
  QCOMPARE(this->propertyValues("Indices"), QList<int>({ 0, 1, 3, 2, 4 }));

  this->Down->click();
  QCOMPARE(this->rows(), QString("0 1 2 3 4 |"));
  QCOMPARE(this->List->currentRow(), 3);
  QCOMPARE(this->propertyValues("Indices"), QList<int>({ 0, 1, 2, 3, 4 }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::moveAcrossSeparator()
{
  QVERIFY(this->createEditor("EnumComponents"));
  this->setPartition({ 0, 1, 2, 3, 4 }, { 1, 5 });
  QSignalSpy indicesSpy(this->Editor.data(), &pqPartitionEditorWidget::partitionIndicesChanged);
  QSignalSpy offsetsSpy(this->Editor.data(), &pqPartitionEditorWidget::partitionOffsetsChanged);

  this->List->setCurrentRow(2);
  this->Up->click();
  QCOMPARE(this->rows(), QString("0 1 | 2 3 4 |"));
  QCOMPARE(indicesSpy.count(), 0);
  QCOMPARE(offsetsSpy.count(), 1);
  QCOMPARE(this->propertyValues("Offsets"), QList<int>({ 2, 5 }));

  this->Down->click();
  QCOMPARE(this->rows(), QString("0 | 1 2 3 4 |"));
  QCOMPARE(this->propertyValues("Offsets"), QList<int>({ 1, 5 }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::loneComponentCannotMove()
{
  QVERIFY(this->createEditor("EnumComponents"));
  this->setPartition({ 0, 1, 2, 3, 4 }, { 1, 2, 5 });
  QCOMPARE(this->rows(), QString("0 | 1 | 2 3 4 |"));
  QCOMPARE(this->buttonStates(0), QString("----"));
  QCOMPARE(this->buttonStates(2), QString("----"));

  this->List->setCurrentRow(2);
  this->Up->click();
  this->Down->click();
  QCOMPARE(this->rows(), QString("0 | 1 | 2 3 4 |"));
  QCOMPARE(this->propertyValues("Offsets"), QList<int>({ 1, 2, 5 }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::addSeparator()
{
  QVERIFY(this->createEditor("EnumComponents"));
  QSignalSpy changeSpy(this->Editor.data(), &pqPropertyWidget::changeAvailable);

  // the first edit also commits the display-only identity indices.
  this->List->setCurrentRow(1);
  this->Add->click();
  QCOMPARE(this->rows(), QString("0 1 | 2 3 4 |"));
  QCOMPARE(this->List->currentRow(), 1);
  QVERIFY(changeSpy.count() > 0);
  QCOMPARE(this->propertyValues("Indices"), QList<int>({ 0, 1, 2, 3, 4 }));
  QCOMPARE(this->propertyValues("Offsets"), QList<int>({ 2, 5 }));
  QVERIFY(!this->Add->isEnabled());
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::removeSeparator()
{
  QVERIFY(this->createEditor("EnumComponents"));
  this->setPartition({ 0, 1, 2, 3, 4 }, { 2, 5 });
  QSignalSpy indicesSpy(this->Editor.data(), &pqPartitionEditorWidget::partitionIndicesChanged);

  this->List->setCurrentRow(2);
  this->Remove->click();
  QCOMPARE(this->rows(), QString("0 1 2 3 4 |"));
  QCOMPARE(this->List->currentRow(), 2);
  QCOMPARE(indicesSpy.count(), 0);
  QCOMPARE(this->propertyValues("Offsets"), QList<int>({ 5 }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::trailingSeparatorIsFixed()
{
  QVERIFY(this->createEditor("EnumComponents"));
  QSignalSpy changeSpy(this->Editor.data(), &pqPropertyWidget::changeAvailable);
  QCOMPARE(this->buttonStates(5), QString("----"));
  this->Up->click();
  this->Down->click();
  this->Add->click();
  this->Remove->click();
  QCOMPARE(this->rows(), QString("0 1 2 3 4 |"));
  QCOMPARE(changeSpy.count(), 0);
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::seriesVisibilityChangesComponentCount()
{
  QVERIFY(this->createEditor("SeriesComponents"));
  QCOMPARE(this->rows(), QString(""));

  auto* components =
    vtkSMStringVectorProperty::SafeDownCast(this->Proxy->GetProperty("Components"));
  components->SetElements({ "a", "1", "b", "0", "c", "1" });
  QCOMPARE(this->rows(), QString("0 1 |"));
  QCOMPARE(this->labels(), QStringList({ "0: a", "1: c" }));

  this->setPartition({ 1, 0 }, { 1, 2 });
  QCOMPARE(this->rows(), QString("1 | 0 |"));

  // the indices would refer to different components, so the partition is
  // cleared
  components->SetElements({ "a", "0", "b", "1", "c", "1" });
  QCOMPARE(this->rows(), QString("0 1 |"));
  QCOMPARE(this->labels(), QStringList({ "0: b", "1: c" }));
  QVERIFY(this->propertyValues("Indices").isEmpty());
  QVERIFY(this->propertyValues("Offsets").isEmpty());

  components->SetElements({ "a", "1", "b", "1", "c", "1" });
  QCOMPARE(this->rows(), QString("0 1 2 |"));
  QCOMPARE(this->labels(), QStringList({ "0: a", "1: b", "2: c" }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::randomEditsKeepPartitionValid()
{
  QVERIFY(this->createEditor("EnumComponents"));
  QToolButton* buttons[] = { this->Up, this->Down, this->Add, this->Remove };
  std::mt19937 rng(12345);
  bool committed = false;
  for (int step = 0; step < 2000; ++step)
  {
    const QString before = this->rows();
    const int row = static_cast<int>(rng() % static_cast<unsigned int>(this->List->count()));
    this->List->setCurrentRow(row);
    QToolButton* button = buttons[rng() % 4];
    const bool enabled = button->isEnabled();
    button->click();
    QVERIFY2(!enabled || this->rows() != before,
      qPrintable(QString("enabled button did nothing: %1 on row %2 of %3")
                   .arg(button->toolTip())
                   .arg(row)
                   .arg(before)));
    committed = committed || enabled;
    if (committed)
    {
      const QString error = this->partitionError(::NUMBER_OF_ENUM_COMPONENTS);
      QVERIFY2(error.isEmpty(),
        qPrintable(QString("%1 after step %2: %3 -> %4")
                     .arg(error)
                     .arg(step)
                     .arg(before)
                     .arg(this->rows())));
    }
  }
  QVERIFY(committed);
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::rescaleButtonsHiddenWithoutCustomRange()
{
  QVERIFY(this->createEditor("EnumComponents"));
  QVERIFY(this->Rescale->isHidden());
  QVERIFY(this->RescaleCustom->isHidden());
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::rescaleDisabledWithoutSelection()
{
  QVERIFY(this->createEditor("RangeComponents"));
  QCOMPARE(this->rows(), QString("0 1 2 |"));
  QVERIFY(!this->Rescale->isHidden());
  QVERIFY(this->List->selectedItems().isEmpty());
  QVERIFY(!this->Rescale->isEnabled());
  QVERIFY(!this->RescaleCustom->isEnabled());
  this->List->setCurrentRow(3);
  QVERIFY(this->Rescale->isEnabled());
  QVERIFY(this->RescaleCustom->isEnabled());
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::editsKeepSelection()
{
  QVERIFY(this->createEditor("RangeComponents"));
  QCOMPARE(this->rows(), QString("0 1 2 |"));
  this->List->setCurrentRow(1);
  QString error = this->selectionError(1);
  QVERIFY2(error.isEmpty(), qPrintable(error));

  // the moved component stays selected
  this->Up->click();
  QCOMPARE(this->rows(), QString("1 0 2 |"));
  error = this->selectionError(0);
  QVERIFY2(error.isEmpty(), qPrintable(error));
  this->Down->click();
  QCOMPARE(this->rows(), QString("0 1 2 |"));
  error = this->selectionError(1);
  QVERIFY2(error.isEmpty(), qPrintable(error));

  // the component before the new separator stays selected
  this->Add->click();
  QCOMPARE(this->rows(), QString("0 1 | 2 |"));
  error = this->selectionError(1);
  QVERIFY2(error.isEmpty(), qPrintable(error));

  // the row that takes the removed separator's place is selected
  this->List->setCurrentRow(2);
  this->Remove->click();
  QCOMPARE(this->rows(), QString("0 1 2 |"));
  error = this->selectionError(2);
  QVERIFY2(error.isEmpty(), qPrintable(error));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::rescaleToDataRange()
{
  QVERIFY(this->createEditor("RangeComponents"));
  this->setPartition({ 0, 2, 1 }, { 2, 3 });
  QCOMPARE(this->rows(), QString("0 2 | 1 |"));
  QSignalSpy changeSpy(this->Editor.data(), &pqPropertyWidget::changeAvailable);

  // an empty CustomRange gets the data range of every group.
  this->List->setCurrentRow(3);
  this->Rescale->click();
  QCOMPARE(this->customRange(), QList<double>({ 0, 1, 0, 0 }));
  QVERIFY(changeSpy.count() > 0);

  // otherwise only the selected group changes; a separator selects the group it closes.
  vtkSMPropertyHelper(this->Proxy, "CustomRange").Set(std::vector<double>{ 5, 6, 7, 8 }.data(), 4);
  this->List->setCurrentRow(2);
  this->Rescale->click();
  QCOMPARE(this->customRange(), QList<double>({ 0, 1, 7, 8 }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::rescaleToCustomRange()
{
  QVERIFY(this->createEditor("RangeComponents"));
  this->setPartition({ 0, 2, 1 }, { 2, 3 });
  QString initialRange;
  // RescaleCustom->click() below blocks in the dialog's exec(): the timer fires
  // in its event loop, so it can fill in the dialog and close it
  QTimer::singleShot(0,
    [&initialRange]()
    {
      auto* dialog =
        qobject_cast<pqRescaleScalarRangeToCustomDialog*>(QApplication::activeModalWidget());
      if (!dialog)
      {
        return;
      }
      auto* minimum = dialog->findChild<QLineEdit*>("MinimumScalar");
      auto* maximum = dialog->findChild<QLineEdit*>("MaximumScalar");
      initialRange = minimum->text() + " " + maximum->text();
      minimum->setText("-2");
      maximum->setText("3.5");
      dialog->findChild<QPushButton*>("RescaleButton")->click();
    });
  this->List->setCurrentRow(0);
  this->RescaleCustom->click();
  // the dialog starts from the data range of the group.
  QCOMPARE(initialRange, QString("0 1"));
  QCOMPARE(this->customRange(), QList<double>({ -2, 3.5, 0, 0 }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::partitionEditClearsCustomRange()
{
  QVERIFY(this->createEditor("RangeComponents"));
  this->setPartition({ 0, 2, 1 }, { 2, 3 });
  vtkSMPropertyHelper(this->Proxy, "CustomRange").Set(std::vector<double>{ 5, 6, 7, 8 }.data(), 4);
  QSignalSpy rangeSpy(this->Editor.data(), &pqPartitionEditorWidget::customRangeChanged);
  this->List->setCurrentRow(0);
  this->Down->click();
  QCOMPARE(this->rows(), QString("2 0 | 1 |"));
  QCOMPARE(rangeSpy.count(), 1);
  QVERIFY(this->customRange().isEmpty());
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::arrayChangeClearsCustomRange()
{
  QVERIFY(this->createEditor("RangeComponents"));
  vtkSMPropertyHelper(this->Proxy, "CustomRange").Set(std::vector<double>{ 5, 6 }.data(), 2);
  QSignalSpy rangeSpy(this->Editor.data(), &pqPartitionEditorWidget::customRangeChanged);
  vtkSMPropertyHelper(this->Proxy, "Array").Set(4, "TextureCoordinates");
  QCOMPARE(this->rows(), QString("0 1 |"));
  QCOMPARE(rangeSpy.count(), 1);
  QVERIFY(this->customRange().isEmpty());
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::seriesVisibilityChangeClearsCustomRange()
{
  QVERIFY(this->createEditor("SeriesComponents"));
  auto* components =
    vtkSMStringVectorProperty::SafeDownCast(this->Proxy->GetProperty("Components"));
  vtkSMPropertyHelper(this->Proxy, "CustomRange").Set(std::vector<double>{ 5, 6 }.data(), 2);
  // from no components, as when loading a state, CustomRange is kept.
  components->SetElements({ "a", "1", "b", "1" });
  QCOMPARE(this->rows(), QString("0 1 |"));
  QCOMPARE(this->customRange(), QList<double>({ 5, 6 }));

  components->SetElements({ "a", "1", "b", "0" });
  QCOMPARE(this->rows(), QString("0 |"));
  QVERIFY(this->customRange().isEmpty());
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::visibleComponentsLimitComponents()
{
  QVERIFY(this->createEditor("VisibleEnumComponents"));
  this->setPartition({ 0, 1, 2, 3, 4 }, { 2, 5 });
  QCOMPARE(this->rows(), QString("0 1 | 2 3 4 |"));
  // index i is the i-th visible component; the partition referred to all 5. A
  // panel edit sets the pending (unchecked) value.
  vtkSMUncheckedPropertyHelper(this->Proxy, "VisibleComponents")
    .Set(std::vector<int>{ 0, 2, 4 }.data(), 3);
  QCOMPARE(this->rows(), QString("0 1 2 |"));
  QCOMPARE(this->labels(), QStringList({ "0: Red", "1: Blue", "2: Extra" }));
  QVERIFY(this->propertyValues("Indices").isEmpty());
  QVERIFY(this->propertyValues("Offsets").isEmpty());
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::visibleComponentsFromPythonKeepPartition()
{
  QVERIFY(this->createEditor("VisibleEnumComponents"));
  // Python sets the partition for the visible components before them.
  this->setPartition({ 0, 1, 2 }, { 1, 3 });
  vtkSMPropertyHelper(this->Proxy, "VisibleComponents").Set(std::vector<int>{ 0, 2, 4 }.data(), 3);
  QCOMPARE(this->rows(), QString("0 | 1 2 |"));
  QCOMPARE(this->propertyValues("Indices"), QList<int>({ 0, 1, 2 }));
  QCOMPARE(this->propertyValues("Offsets"), QList<int>({ 1, 3 }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::naturalOrderPlacesMagnitude()
{
  // a RadViz representation of Wavelet -> Gradient -> Calculator: the table's
  // columns are RTData, Gradient_X/Y/Z, Gradient_Magnitude, Result.
  vtkSMSessionProxyManager* pxm = this->Server->proxyManager();
  vtkSmartPointer<vtkSMProxy> wavelet;
  wavelet.TakeReference(pxm->NewProxy("sources", "RTAnalyticSource"));
  wavelet->UpdateVTKObjects();
  vtkSmartPointer<vtkSMProxy> gradient;
  gradient.TakeReference(pxm->NewProxy("filters", "Gradient"));
  vtkSMPropertyHelper(gradient, "Input").Set(wavelet);
  vtkSMPropertyHelper(gradient, "ScalarArray").Set(4, "RTData");
  gradient->UpdateVTKObjects();
  this->Input.TakeReference(pxm->NewProxy("filters", "Calculator"));
  vtkSMPropertyHelper(this->Input, "Input").Set(gradient);
  vtkSMPropertyHelper(this->Input, "Function").Set("RTData");
  vtkSMPropertyHelper(this->Input, "ResultArrayName").Set("Result");
  this->Input->UpdateVTKObjects();
  vtkSMSourceProxy::SafeDownCast(this->Input)->UpdatePipeline();
  this->Proxy.TakeReference(pxm->NewProxy("representations", "RadVizRepresentation"));
  QVERIFY(this->Proxy);
  vtkSMPropertyHelper(this->Proxy, "Input").Set(this->Input);
  this->Proxy->UpdateVTKObjects();
  vtkSMStringVectorProperty::SafeDownCast(this->Proxy->GetProperty("SeriesVisibility"))
    ->SetElements({ "Gradient_X", "1", "Gradient_Magnitude", "1", "Result", "1" });
  vtkSMPropertyGroup* group = ::partitionsGroup(this->Proxy);
  QVERIFY(group);
  this->Editor = new pqPartitionEditorWidget(this->Proxy, group);
  this->List = this->Editor->findChild<QListWidget*>();
  QVERIFY(this->List);
  // the magnitude follows the components, before Result.
  QCOMPARE(this->labels(), QStringList({ "0: Gradient_X", "1: Gradient_Magnitude", "2: Result" }));
}

//-----------------------------------------------------------------------------
void PartitionEditorWidgetTester::compositeInputRescales()
{
  // a RadViz representation of Wavelet -> Gradient -> GroupDataSets, with a
  // series not in the input.
  vtkSMSessionProxyManager* pxm = this->Server->proxyManager();
  vtkSmartPointer<vtkSMProxy> wavelet;
  wavelet.TakeReference(pxm->NewProxy("sources", "RTAnalyticSource"));
  wavelet->UpdateVTKObjects();
  vtkSmartPointer<vtkSMProxy> gradient;
  gradient.TakeReference(pxm->NewProxy("filters", "Gradient"));
  vtkSMPropertyHelper(gradient, "Input").Set(wavelet);
  vtkSMPropertyHelper(gradient, "ScalarArray").Set(4, "RTData");
  gradient->UpdateVTKObjects();
  this->Input.TakeReference(pxm->NewProxy("filters", "GroupDataSets"));
  vtkSMPropertyHelper(this->Input, "Input").Set(gradient);
  this->Input->UpdateVTKObjects();
  auto* input = vtkSMSourceProxy::SafeDownCast(this->Input);
  input->UpdatePipeline();
  QVERIFY(input->GetDataInformation()->IsCompositeDataSet());
  vtkPVArrayInformation* gradientInfo =
    input->GetDataInformation()->GetArrayInformation("Gradient", vtkDataObject::POINT);
  QVERIFY(gradientInfo);
  double x[2];
  double y[2];
  gradientInfo->GetComponentFiniteRange(0, x);
  gradientInfo->GetComponentFiniteRange(1, y);

  this->Proxy.TakeReference(pxm->NewProxy("representations", "RadVizRepresentation"));
  QVERIFY(this->Proxy);
  vtkSMPropertyHelper(this->Proxy, "Input").Set(this->Input);
  vtkSMStringVectorProperty::SafeDownCast(this->Proxy->GetProperty("SeriesVisibility"))
    ->SetElements({ "Gradient_X", "1", "Gradient_Y", "1", "Missing", "1" });
  vtkSMPropertyHelper(this->Proxy, "PartitionIndices").Set(std::vector<int>{ 0, 1, 2 }.data(), 3);
  vtkSMPropertyHelper(this->Proxy, "PartitionOffsets").Set(std::vector<int>{ 1, 2, 3 }.data(), 3);
  this->Proxy->UpdateVTKObjects();
  vtkSMPropertyGroup* group = ::partitionsGroup(this->Proxy);
  QVERIFY(group);
  this->Editor = new pqPartitionEditorWidget(this->Proxy, group);
  this->List = this->Editor->findChild<QListWidget*>();
  this->Rescale = this->Editor->findChild<QToolButton*>("Rescale");
  this->RescaleCustom = this->Editor->findChild<QToolButton*>("RescaleCustom");
  QVERIFY(this->List && this->Rescale && this->RescaleCustom);
  QCOMPARE(this->labels(), QStringList({ "0: Gradient_X", "1: Gradient_Y", "2: Missing" }));

  // the missing series has no data range: Rescale is disabled for its group,
  // and Rescale Custom for any group, as it needs every group's data range
  // without a custom range.
  this->List->setCurrentRow(4);
  QVERIFY(!this->Rescale->isEnabled());
  QVERIFY(!this->RescaleCustom->isEnabled());
  this->List->setCurrentRow(0);
  QVERIFY(this->Rescale->isEnabled());
  QVERIFY(!this->RescaleCustom->isEnabled());

  // with a custom range, the groups of the composite input's series rescale to their data range.
  vtkSMPropertyHelper(this->Proxy, "CustomRange")
    .Set(std::vector<double>{ 0, 1, 0, 1, 0, 1 }.data(), 6);
  QVERIFY(this->RescaleCustom->isEnabled());
  this->Rescale->click();
  this->List->setCurrentRow(2);
  this->Rescale->click();
  QCOMPARE(this->customRange(), QList<double>({ x[0], x[1], y[0], y[1], 0, 1 }));
}

//-----------------------------------------------------------------------------
int PartitionEditorWidget(int argc, char* argv[])
{
  QApplication app(argc, argv);
  // keep QTest arguments away from ParaView's command line parser.
  char disableRegistry[] = "--dr";
  char* coreArgv[] = { argv[0], disableRegistry, nullptr };
  int coreArgc = 2;
  pqApplicationCore core(coreArgc, coreArgv);
  pqServer* server = core.getObjectBuilder()->createServer(
    pqServerResource(QString(vtkSMSession::GetBuiltinName())));

  PartitionEditorWidgetTester tester(server);
  // QTest rejects the --dr the test driver passes.
  return QTest::qExec(&tester, 1, argv);
}
