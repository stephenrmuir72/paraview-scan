#ifndef pqShaderEditorButton_h
#define pqShaderEditorButton_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include "vtkImageVolumeRepresentation.h"
#include "vtkSMProxy.h"
#include <qglobal.h>

class PQAPPLICATIONCOMPONENTS_EXPORT pqShaderEditorButton : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  explicit pqShaderEditorButton(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqShaderEditorButton() override;

    void editorClosed();

public Q_SLOTS:
  void buttonReleased();

private:
  Q_DISABLE_COPY(pqShaderEditorButton)

  struct pqInternals;
  pqInternals *Internals;
};

#endif