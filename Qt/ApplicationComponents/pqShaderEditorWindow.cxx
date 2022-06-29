#include "pqShaderEditorWindow.h"
#include "pqPropertiesPanel.h"
#include "pqShaderEditorButton.h"
#include "vtkImageVolumeRepresentation.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QFontMetrics>
#include <QGridLayout>
#include <QPushButton>
#include <QTextEdit>
#include <qcursor.h>
#include <qfontmetrics.h>
#include <qgridlayout.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qtextformat.h>
#include <qwidget.h>

struct pqShaderEditorWindow::pqInternals
{
  QTextEdit* TextField;
  QGridLayout* Layout;
  pqShaderEditorButton* ParentCaller;
  vtkSMPropertyGroup* SMGroup;
  vtkSMProxy* Proxy;
  QPushButton* SetShaderButton;
  QPushButton* GetShaderButton;
};

pqShaderEditorWindow::pqShaderEditorWindow(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup,
  pqShaderEditorButton* shaderButton, QWidget* parent)
  : QWidget(parent)
{
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setWindowTitle("Paraview - Shader code editor");
  Internals = new pqInternals;
  Internals->Layout = new QGridLayout(this);
  Internals->TextField = new QTextEdit(this);
  Internals->SetShaderButton = new QPushButton(tr("Set shader code"), this);
  Internals->GetShaderButton = new QPushButton(tr("Fetch shader code"), this);

  Internals->Layout->addWidget(Internals->TextField, 0, 0, 1, -1);
  Internals->Layout->addWidget(Internals->SetShaderButton, 1, 0);
  Internals->Layout->addWidget(Internals->GetShaderButton, 1, 1);

  Internals->Proxy = proxy;
  Internals->SMGroup = smgroup;
  Internals->ParentCaller = shaderButton;
  connect(Internals->SetShaderButton, &QPushButton::released, this,
    &pqShaderEditorWindow::setShaderReleased);
  connect(Internals->GetShaderButton, &QPushButton::released, this,
    &pqShaderEditorWindow::getShaderReleased);
  connect(Internals->TextField, &QTextEdit::cursorPositionChanged, this,
    &pqShaderEditorWindow::cursorPositionUpdated);
  this->show();
}

void pqShaderEditorWindow::cursorPositionUpdated() 
{
  QTextCursor cursor = Internals->TextField->textCursor();
  cursor.select(QTextCursor::LineUnderCursor);
  QTextCharFormat textformat = Internals->TextField->currentCharFormat();
  textformat.setBackground(QBrush(QColor(247, 252, 134, 200)));
  QList<QTextEdit::ExtraSelection> extraselections;
  extraselections.append({ cursor, textformat });
  Internals->TextField->setExtraSelections(extraselections);
}

void pqShaderEditorWindow::setShaderReleased()
{
  vtkSMPropertyHelper(Internals->SMGroup->GetProperty("SetShaderCode"))
    .Set(Internals->TextField->toPlainText().toStdString().c_str());
  Internals->Proxy->UpdateVTKObjects();
}

void pqShaderEditorWindow::getShaderReleased()
{
  Internals->Proxy->UpdatePropertyInformation();
  std::string shader =
    vtkSMPropertyHelper(Internals->SMGroup->GetProperty("GetShaderCode")).GetAsString();
  Internals->TextField->setText(shader.c_str());
  // compute max line width and height
  QString shaderQ(shader.c_str());

  QRect recScreen = QApplication::desktop()->screenGeometry();
  QFontMetrics fontmetrics = fontMetrics();
  QSize textSize = fontmetrics.size(0, shaderQ);

  textSize.setWidth(std::min(textSize.width(), (int)(recScreen.width() * 0.5)));
  textSize.setHeight(std::min(textSize.height(), (int)(recScreen.height() * 0.8)));

  QSize widgetSize(textSize.width() + 30, textSize.height() + 50);

  this->resize(widgetSize);
  Internals->TextField->resize(textSize);
}

pqShaderEditorWindow::~pqShaderEditorWindow()
{
  if (Internals->ParentCaller)
  {
    Internals->ParentCaller->editorClosed();
  }
  // we reset the shader when the window is closed
  vtkSMPropertyHelper(Internals->SMGroup->GetProperty("SetShaderCode")).Set("");
  Internals->Proxy->UpdateVTKObjects();
  delete Internals->SetShaderButton;
  delete Internals->GetShaderButton;
  delete Internals->TextField;
  delete Internals;
}