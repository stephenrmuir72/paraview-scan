/*=========================================================================

   Program: ParaView
   Module:    pqServer.h

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

=========================================================================*/
#ifndef pqServer_h
#define pqServer_h

class pqServerResource;
class vtkClientSession;
class vtkPVServerInformation;
class vtkSMProxySelectionModel;
class vtkSMSessionProxyManager;
class vtkObject;

#include "pqCoreModule.h" // for exports
#include "pqServerManagerModelItem.h"
#include "vtkType.h" // for vtkTypeUInt32

#include <QPointer>
#include <memory> // for std::unique_ptr

/**
 * pqServer (should be renamed to pqSession) is a pqServerManagerModelItem
 * subclass that represents a vtkSMSession. Besides providing API to access
 * vtkSMSession, it also performs some initialization of session-related
 * proxies such as time-keeper and global-mapper-properties proxies.
 */
class PQCORE_EXPORT pqServer : public pqServerManagerModelItem
{
  Q_OBJECT;
  using Superclass = pqServerManagerModelItem;

public:
  pqServer(vtkClientSession* session, QObject* parent = nullptr);
  ~pqServer() override;

  /**
   * Returns the session instance which the pqServer represents.
   */
  vtkClientSession* session() const;

  /**
   * Returns the connection id for the server connection.
   */
  vtkTypeUInt32 sessionID() const;

  ///@{
  /**
   * Get/Set the pqServerResource associated with this session.
   */
  const pqServerResource& resource() const;
  void setResource(const pqServerResource& server_resource);
  ///@}

  /**
   * Returns the proxy manager for this session.
   */
  vtkSMSessionProxyManager* proxyManager() const;

  /**
   * Sources selection model is used to keep track of sources currently
   * selected on this session/server-connection.
   */
  vtkSMProxySelectionModel* activeSourcesSelectionModel() const;

  /**
   * View selection model is used to keep track of active view.
   */
  vtkSMProxySelectionModel* activeViewSelectionModel() const;

  /**
   * Return the number of data server partitions on this server connection. A
   * convenience method.
   */
  int numberOfRanks() const;

  /**
   * Returns is this connection is a connection to a remote server or a
   * built-in server.
   */
  bool isRemote() const;

  /**
   * Returns the vtkPVServerInformation object which contains information about
   * the command line options specified on the remote server, if any.
   */
  vtkPVServerInformation* serverInformation() const;

Q_SIGNALS:
  /**
   * Fired when the name of the proxy is changed.
   */
  void nameChanged(pqServerManagerModelItem*);

  /**
   * Fired about 5 minutes before the server timesout. This signal will not be
   * fired at all if server timeout < 5 minutes. The server timeout is
   * specified by --timeout option on the server process.
   * This is not fired if timeout is not specified on the server process.
   */
  void fiveMinuteTimeoutWarning();

  /**
   * Fired about 1 minute before the server timesout.
   * The server timeout is specified by --timeout option on the server process.
   * This is not fired if timeout is not specified on the server process.
   */
  void finalTimeoutWarning();

  /**
   * Fired if any server side crash or disconnection occurred.
   */
  void serverSideDisconnected();

protected Q_SLOTS:
  /**
   * Called by vtkSMSessionClient is any communication error occurred with the
   * server. This usually mean that the server side is dead.
   */
  void onConnectionLost(vtkObject*, unsigned long, void*, void*);

private:
  Q_DISABLE_COPY(pqServer)

  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif // !pqServer_h
