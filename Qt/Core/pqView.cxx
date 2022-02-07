/*=========================================================================

   Program: ParaView
   Module:    pqView.cxx

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
#include "pqView.h"

// ParaView Server Manager includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

// Qt includes.
#include <QList>
#include <QPointer>
#include <QWidget>
#include <QtDebug>

// ParaView includes.
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProgressManager.h"
#include "pqQVTKWidget.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimer.h"

#include <cmath>

template <class T>
inline uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}

class pqViewInternal
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<QWidget> Widget;
  bool WidgetCreated;

  // List of representation shown by this view.
  QList<QPointer<pqRepresentation>> Representations;

  pqViewInternal()
    : WidgetCreated(false)
  {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  }

  ~pqViewInternal() { delete this->Widget; }

  pqTimer RenderTimer;
};

//-----------------------------------------------------------------------------
pqView::pqView(const QString& group, const QString& name, vtkSMViewProxy* view, pqServer* server,
  QObject* _parent /*=nullptr*/)
  : pqProxy(group, name, view, server, _parent)
{
  this->Internal = new pqViewInternal();

  // Listen to updates on the Representations property.
  this->Internal->VTKConnect->Connect(view->GetProperty("Representations"),
    vtkCommand::ModifiedEvent, this, SLOT(onRepresentationsChanged()));

  // Fire updateDataEvent
  this->Internal->VTKConnect->Connect(
    view, vtkCommand::UpdateDataEvent, this, SIGNAL(updateDataEvent()));

  this->Internal->RenderTimer.setSingleShot(true);
  this->Internal->RenderTimer.setInterval(1);
  QObject::connect(&this->Internal->RenderTimer, SIGNAL(timeout()), this, SLOT(forceRender()));

#if 0 // FIXME: ASYNC
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smModel, SIGNAL(representationAdded(pqRepresentation*)), this,
    SLOT(representationCreated(pqRepresentation*)));
#endif

  // If the render module already has some representations in it when it is
  // registered, this method will detect them and sync the GUI state with the
  // SM state.
  this->onRepresentationsChanged();
}

//-----------------------------------------------------------------------------
pqView::~pqView()
{
  Q_FOREACH (pqRepresentation* disp, this->Internal->Representations)
  {
    if (disp)
    {
      disp->setView(nullptr);
    }
  }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqView::getViewProxy() const
{
  return vtkSMViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
QWidget* pqView::widget()
{
  if (this->Internal->WidgetCreated == false)
  {
    this->Internal->Widget = this->createWidget();
    this->Internal->WidgetCreated = true;
    if (this->Internal->Widget)
    {
      this->Internal->Widget->setObjectName("Viewport");
    }
  }
  return this->Internal->Widget;
}

//-----------------------------------------------------------------------------
void pqView::cancelPendingRenders()
{
  this->Internal->RenderTimer.stop();
}

//-----------------------------------------------------------------------------
void pqView::emitSelectionSignals(bool frustum)
{
  Q_EMIT selectionModeChanged(frustum);
}

//-----------------------------------------------------------------------------
void pqView::render()
{
  this->Internal->RenderTimer.start();
}

//-----------------------------------------------------------------------------
void pqView::forceRender()
{
  // avoid calling render if the widget isn't valid, i.e. if the context isn't
  // ready yet. This is due to asynchronous initialization of the context by
  // the pqQVTKWidget class.
  pqQVTKWidget* qwdg = qobject_cast<pqQVTKWidget*>(this->widget());
  if (qwdg != nullptr && !qwdg->isValid())
  {
    return;
  }

  // cancel any pending renders, if this method is called directly.
  this->cancelPendingRenders();
  vtkSMViewProxy* view = this->getViewProxy();
  if (view)
  {
    view->StillRender();
  }
}

//-----------------------------------------------------------------------------
bool pqView::hasRepresentation(pqRepresentation* repr) const
{
  return this->Internal->Representations.contains(repr);
}

//-----------------------------------------------------------------------------
int pqView::getNumberOfRepresentations() const
{
  return this->Internal->Representations.size();
}

//-----------------------------------------------------------------------------
int pqView::getNumberOfVisibleRepresentations() const
{
  int count = 0;
  for (int i = 0; i < this->Internal->Representations.size(); i++)
  {
    pqRepresentation* repr = this->Internal->Representations[i];
    if (repr && repr->isVisible())
    {
      count++;
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
int pqView::getNumberOfVisibleDataRepresentations() const
{
  int count = 0;
  for (int i = 0; i < this->Internal->Representations.size(); i++)
  {
    pqDataRepresentation* repr =
      qobject_cast<pqDataRepresentation*>(this->Internal->Representations[i]);
    if (repr && repr->isVisible())
    {
      count++;
    }
  }

  return count;
}

//-----------------------------------------------------------------------------
pqRepresentation* pqView::getRepresentation(int index) const
{
  if (index >= 0 && index < this->Internal->Representations.size())
  {
    return this->Internal->Representations[index];
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
QList<pqRepresentation*> pqView::getRepresentations() const
{
  QList<pqRepresentation*> list;
  Q_FOREACH (pqRepresentation* disp, this->Internal->Representations)
  {
    if (disp)
    {
      list.push_back(disp);
    }
  }
  return list;
}

//-----------------------------------------------------------------------------
void pqView::onRepresentationsChanged()
{
#if 0 // FIXME: ASYNC
  // Determine what changed. Add the new Representations and remove the old
  // ones. Make sure new Representations have a reference to this render module.
  // Remove the reference to this render module in the removed Representations.
  QList<QPointer<pqRepresentation>> currentReprs;
  vtkSMProxyProperty* prop =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("Representations"));
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();

  unsigned int max = prop->GetNumberOfProxies();
  for (unsigned int cc = 0; cc < max; ++cc)
  {
    vtkSMProxy* proxy = prop->GetProxy(cc);
    if (!proxy)
    {
      continue;
    }
    pqRepresentation* repr = smModel->findItem<pqRepresentation*>(proxy);
    if (!repr)
    {
      continue;
    }
    currentReprs.append(QPointer<pqRepresentation>(repr));
    if (!this->Internal->Representations.contains(repr))
    {
      // Update the render module pointer in the repr.
      repr->setView(this);
      this->Internal->Representations.append(QPointer<pqRepresentation>(repr));
      QObject::connect(
        repr, SIGNAL(visibilityChanged(bool)), this, SLOT(onRepresentationVisibilityChanged(bool)));
      Q_EMIT this->representationAdded(repr);
      Q_EMIT this->representationVisibilityChanged(repr, repr->isVisible());
    }
  }

  QList<QPointer<pqRepresentation>>::Iterator iter = this->Internal->Representations.begin();
  while (iter != this->Internal->Representations.end())
  {
    if (*iter && !currentReprs.contains(*iter))
    {
      pqRepresentation* repr = (*iter);
      // Remove the render module pointer from the repr.
      repr->setView(nullptr);
      iter = this->Internal->Representations.erase(iter);
      QObject::disconnect(repr, nullptr, this, nullptr);
      Q_EMIT this->representationVisibilityChanged(repr, false);
      Q_EMIT this->representationRemoved(repr);
    }
    else
    {
      ++iter;
    }
  }
#endif
}

//-----------------------------------------------------------------------------
void pqView::representationCreated(pqRepresentation* repr)
{
  vtkSMProxyProperty* prop =
    vtkSMProxyProperty::SafeDownCast(this->getProxy()->GetProperty("Representations"));
  if (prop->IsProxyAdded(repr->getProxy()))
  {
    repr->setView(this);
    this->Internal->Representations.append(repr);
    QObject::connect(
      repr, SIGNAL(visibilityChanged(bool)), this, SLOT(onRepresentationVisibilityChanged(bool)));
    Q_EMIT this->representationAdded(repr);
  }
}

//-----------------------------------------------------------------------------
void pqView::onRepresentationVisibilityChanged(bool visible)
{
  pqRepresentation* disp = qobject_cast<pqRepresentation*>(this->sender());
  if (disp)
  {
    Q_EMIT this->representationVisibilityChanged(disp, visible);
  }
}

//-----------------------------------------------------------------------------
QSize pqView::getSize()
{
  QWidget* wdg = this->widget();
  return wdg ? wdg->size() : QSize(0, 0);
}

//-----------------------------------------------------------------------------
QWidget* pqView::createWidget()
{
  pqQVTKWidget* vtkwidget = new pqQVTKWidget();
  vtkwidget->setViewProxy(this->proxy());
  vtkwidget->setRenderWindow(this->getViewProxy()->GetRenderWindow());
  vtkwidget->setContextMenuPolicy(Qt::NoContextMenu);
  vtkwidget->installEventFilter(this);
  return vtkwidget;
}

//-----------------------------------------------------------------------------
bool pqView::canDisplay(pqOutputPort* opPort) const
{
#if 0 // FIXME: ASYNC
  pqPipelineSource* source = opPort ? opPort->getSource() : nullptr;
  vtkSMSourceProxy* sourceProxy =
    source ? vtkSMSourceProxy::SafeDownCast(source->getProxy()) : nullptr;
  if (!opPort || !sourceProxy || opPort->getServer()->getResource().scheme() == "catalyst")
  {
    return false;
  }

  return this->getViewProxy()->CanDisplayData(sourceProxy, opPort->getPortNumber());
#endif
  return false;
}
