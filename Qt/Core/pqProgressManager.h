/*=========================================================================

   Program: ParaView
   Module:    pqProgressManager.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqProgressManager_h
#define pqProgressManager_h

#include "pqCoreModule.h" // for exports
#include <QObject>
#include <memory> // for std::unique_ptr

/**
 * @class pqProgressManager
 * @brief helper class to consolidate various progress notifications
 *
 * A ParaView client can receive a multitude of progress notifications. Each
 * service can reports execution progress for algorithms executing on that
 * service. Each session is connected to multiple services and there can be
 * multiple sessions. This class makes it easy to consolidate progress events
 * from various sources.
 *
 */
class PQCORE_EXPORT pqProgressManager : public QObject
{
  Q_OBJECT
public:
  pqProgressManager(QObject* parent = nullptr);
  ~pqProgressManager() override;

  /**
   * Locks progress to respond to progress signals set by the \c object alone.
   * All signals sent by other objects are ignored until Unlock is called.
   */
  void lockProgress(QObject* object);

  /**
   * Releases the progress lock.
   */
  void unlockProgress(QObject* object);

  /**
   * Returns if the progress is currently locked by any object.
   */
  bool isLocked() const;

public Q_SLOTS:
  /**
   * Update progress. The progress must be enbled by calling
   * enableProgress(true) before calling  this method for the progress to be
   * updated.
   */
  void setProgress(const QString& message, int progress);

  /**
   * Enables progress.
   */
  void setEnableProgress(bool);

  /**
   * Convenience slots that simply call setEnableProgress().
   */
  void beginProgress() { this->setEnableProgress(true); }
  void endProgress() { this->setEnableProgress(false); }

  /**
   * Enables abort.
   */
  void setEnableAbort(bool);

  /**
   * fires abort(). Must be called by the GUI that triggers abort.
   */
  void triggerAbort();

Q_SIGNALS:
  /**
   * Emitted to trigger an abort.
   */
  void abort();

  /**
   * Fired for each progress message. \c progress is in range [0, 100].
   */
  void progress(const QString& message, int progress);

  void enableProgress(bool);

  void enableAbort(bool);

private:
  Q_DISABLE_COPY(pqProgressManager);
  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
