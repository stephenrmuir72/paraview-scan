/*=========================================================================

   Program: ParaView
   Module:    pqProxy.h

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
#ifndef pqProxy_h
#define pqProxy_h

#include "pqServerManagerModelItem.h"
#include <memory> // for std::unique_ptr

class pqServer;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMSessionProxyManager;

/**
 * This class represents any registered Server Manager proxy.
 * It keeps essential information to locate the proxy as well as additional
 * metadata such as user-specified label.
 */
class PQCORE_EXPORT pqProxy : public pqServerManagerModelItem
{
  Q_OBJECT;
  using Superclass = pqServerManagerModelItem;

public:
  /**
   * The modification state of this proxy
   */
  enum ModifiedState
  {
    UNINITIALIZED,
    MODIFIED,
    UNMODIFIED
  };

  pqProxy(const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server,
    QObject* parent = nullptr);
  ~pqProxy() override;

  /**
   * Get the server on which this proxy exists.
   */
  pqServer* server() const;
  pqServer* getServer() const { return this->server(); }

  /**
   * This is a convenience method. It re-registers the underlying proxy with
   * the requested new name under the same group. Then it unregisters the proxy
   * from the group with the old name. This operation is understood as renaming
   * the proxy, since as a consequence, this pqProxy's \c SMName changes.
   */
  void rename(const QString& newname);

  /**
   * Get the name with which this proxy is registered on the server manager. A
   * proxy can be registered with more than one name on the Server Manager.
   * This is the name/group which this pqProxy stands for.
   */
  const QString& registrationName() const;
  const QString& registrationGroup() const;

  /**
   * Get the vtkSMProxy this object stands for.
   * This can never be nullptr. A pqProxy always represents one and only one
   * Server Manager proxy.
   */
  vtkSMProxy* proxy() const;
  vtkSMProxy* getProxy() const { return this->proxy(); }

  /**
   * \brief
   *   Gets whether or not the source has been modified.
   * \return
   *   True if the source has been modified.
   */
  ModifiedState modifiedState() const;

  /**
   * \brief
   *   Sets whether or not the source has been modified.
   * \param modified True if the source has been modified.
   */
  void setModifiedState(ModifiedState modified);

  /**
   * Returns the hints for this proxy, if any. May returns nullptr if no hints
   * are defined.
   */
  vtkPVXMLElement* xmlHints() const;

  //@{
  /**
   * convert proxy documentation from RST to HTML (so that it can be used in Qt)
   */
  static std::string rstToHtml(const char* rstStr);
  static QString rstToHtml(const QString& rstStr);
  //@}

  /**
   * Returns the proxy manager by calling this->getProxy()->GetProxyManager();
   */
  vtkSMSessionProxyManager* proxyManager() const;

Q_SIGNALS:
  /**
   * Fired when the name of the proxy is changed.
   */
  void nameChanged(pqServerManagerModelItem*);

  /**
   * Fired when the modified status changes for the proxy.
   */
  void modifiedStateChanged(pqServerManagerModelItem*);

  /**
   * Fired when annotations on the proxy change.
   */
  void annotationsChanged(pqServerManagerModelItem*);

protected:
  /**
   * Use this method to initialize the pqObject state using the
   * underlying vtkSMProxy. This needs to be done only once,
   * after the object has been created.
   */
  virtual void initialize();

private Q_SLOTS:
  void onAnnotationsChanged();

private:
  class pqInternals;
  std::unique_ptr<pqInternals> Internals;

  /**
   * used when proxy is renamed.
   */
  void setRegistrationName(const QString& name);
  friend class pqServerManagerModel;
};

#endif
