// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraWidgetManager_h
#define pqCameraWidgetManager_h

#include "pqComponentsModule.h"

#include <QObject>

#include <memory> // for unique_ptr

/**
 * pqCameraWidgetManager is the place to enhance camera widget within
 * ParaView.
 *
 * It responds to pqCameraWidgetViewLink creation/deletion. When a
 * link is created, a callback is added to the widget's interactor
 * that makes the widget's camera point toward the center of the
 * active selection (if any) on key 'p' pressed.
 *
 * @sa pqCameraWidgetViewLink pqLinksModel
 */
class PQCOMPONENTS_EXPORT pqCameraWidgetManager : public QObject
{
  Q_OBJECT

public:
  pqCameraWidgetManager(QObject* parent = nullptr);
  ~pqCameraWidgetManager() override;

private Q_SLOTS:
  /**
   * Called when pqLinkModel creates a link, to add event callback.
   */
  void onLinkAdded(int linkType, const QString& name);

  /**
   * Called when pqLinkModel removes a link, to remove event callback.
   */
  void onLinkRemoved(const QString& name);

private:
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};
#endif
