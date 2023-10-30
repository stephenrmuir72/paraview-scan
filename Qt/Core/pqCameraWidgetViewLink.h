// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraWidgetViewLink_h
#define pqCameraWidgetViewLink_h

#include "pqCoreModule.h"
#include <QObject>

#include <memory> // for unique_ptr

class pqRenderView;
class vtkCamera3DRepresentation;
class vtkCamera3DWidget;
class vtkPVXMLElement;

/**
 * pqCameraWidgetViewLink is used by pqLinksModel to create
 * camera widget view links, which are ViewLink without property
 * synchronization. This allows render views to call render when
 * interacting with the camera widget or the camera itself.
 */
class PQCORE_EXPORT pqCameraWidgetViewLink : public QObject
{
  Q_OBJECT;
  typedef QObject Superclass;

public:
  pqCameraWidgetViewLink(pqRenderView* displayView, pqRenderView* linkedView);
  ~pqCameraWidgetViewLink() override;

  /**
   * Get the widget representation of the camera widget view link.
   */
  vtkCamera3DRepresentation* getRepresentation() const;
  /**
   * Get the widget of the camera widget view link.
   */
  vtkCamera3DWidget* getWidget() const;

  // Save this camera widget view link in xml node
  virtual void saveXMLState(vtkPVXMLElement* xml);

private:
  struct pqInternal;
  std::unique_ptr<pqInternal> Internal;
};

#endif
