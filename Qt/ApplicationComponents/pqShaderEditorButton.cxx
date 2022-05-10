#include "pqShaderEditorButton.h"
#include "vtkImageVolumeRepresentation.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "pqPropertiesPanel.h"

#include "pqShaderEditorWindow.h"

#include <QAction>
#include <QGridLayout>
#include <QPushButton>
#include <qpushbutton.h>
#include <qwidget.h>

struct pqShaderEditorButton::pqInternals
{
public:
  vtkImageVolumeRepresentation* VolumeRepr;
  QPushButton *windowPB;
  QWidget *Parent;
  QWidget *WindowEditor;
  vtkSMProxy *Proxy;
  vtkSMPropertyGroup *SMGroup;
};

void pqShaderEditorButton::buttonReleased()
{
  if(!Internals->WindowEditor)
  {
    Internals->WindowEditor = new pqShaderEditorWindow(Internals->Proxy, Internals->SMGroup, this, nullptr);
  }
}

void pqShaderEditorButton::editorClosed()
{
  Internals->WindowEditor = nullptr;
}

pqShaderEditorButton::pqShaderEditorButton(
  vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parent) :
  Superclass(proxy, parent)
{
  QGridLayout* layout = new QGridLayout(this);
  layout->setMargin(pqPropertiesPanel::suggestedMargin());
  layout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  layout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

  Internals = new pqInternals;
  Internals->Proxy = proxy;
  Internals->SMGroup = group;
  Internals->VolumeRepr = vtkImageVolumeRepresentation::SafeDownCast(proxy);
  Internals->windowPB = new QPushButton(tr("Edit shader code"), this);
  Internals->WindowEditor = nullptr;
  layout->addWidget(Internals->windowPB, 0, 0);

  Internals->windowPB->show();
  connect(Internals->windowPB, &QPushButton::released, this, &pqShaderEditorButton::buttonReleased);
}

pqShaderEditorButton::~pqShaderEditorButton()
{
  delete Internals;
}