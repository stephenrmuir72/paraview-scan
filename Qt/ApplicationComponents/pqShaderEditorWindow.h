#ifndef pqShaderEditorWindow_h
#define pqShaderEditorWindow_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include "pqShaderEditorButton.h"
#include "vtkImageVolumeRepresentation.h"
#include "vtkSMProxy.h"
#include <QWidget>
#include <qglobal.h>
#include "pqShaderEditorButton.h"

class PQAPPLICATIONCOMPONENTS_EXPORT pqShaderEditorWindow : public QWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  explicit pqShaderEditorWindow(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, pqShaderEditorButton* shaderButton, QWidget* parent = nullptr);
  ~pqShaderEditorWindow() override;

public Q_SLOTS:
    void setShaderReleased();
    void getShaderReleased();
    void cursorPositionUpdated();

private:
    struct pqInternals;
    pqInternals *Internals;
};

#endif