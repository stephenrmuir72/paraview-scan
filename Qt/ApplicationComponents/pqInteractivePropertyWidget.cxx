/*=========================================================================

   Program: ParaView
   Module:  pqInteractivePropertyWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqInteractivePropertyWidget.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderViewBase.h"
#include "pqRepresentation.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QtDebug>

#include <cassert>

//-----------------------------------------------------------------------------
namespace details
{
/**
 * When the widget is a subproxy the pqInteractivePropertyWidget::proxy member
 * does not return the proxy of the actual filter in the pipeline. This function
 * tries to get it by getting the very first consumer.
 */
vtkSMSourceProxy* GetTrueSourceInPipeline(vtkSMProxy* current)
{
  vtkSMProxy* res = nullptr;
  if (current != nullptr)
  {
    res = current;
    vtkSMProxy* parent = current;
    if (current->GetNumberOfConsumers())
    {
      parent = current->GetConsumerProxy(0);
    }

    while (parent != res && parent->GetNumberOfConsumers())
    {
      res = parent;
      parent = res->GetConsumerProxy(0);
    }
  }

  return vtkSMSourceProxy::SafeDownCast(res);
}
}

//-----------------------------------------------------------------------------
struct pqInteractivePropertyWidget::SourceTracker
{
  bool IsVisible = false;
  pqPipelineSource* Proxy = nullptr;
  QMetaObject::Connection RepresentationObserver;

  // Try to find the representation of our current Proxy in the view. If found
  // we link its visibility property to trigger an update of the widget visibility.
  // If not found we wait for the representation to be created. Once it is we can
  // link its visibility property.
  void RegisterRepresentationPromise(pqInteractivePropertyWidget* self, pqView* view)
  {
    if (this->Proxy)
    {
      pqDataRepresentation* representation = this->Proxy->getRepresentation(view);
      if (representation)
      {
        this->IsVisible = representation->isVisible();
        QObject::connect(representation, &pqDataRepresentation::visibilityChanged,
          [this, self, representation](bool vis) {
            if (representation->getView())
            {
              this->IsVisible = vis;
              self->updateWidgetVisibility();
            }
          });
      }
      else
      {
        this->IsVisible = false;

        this->RepresentationObserver = QObject::connect(
          view, &pqView::representationAdded, [this, self, view](pqRepresentation* repr) {
            if (auto* dataRepr = qobject_cast<pqDataRepresentation*>(repr))
            {
              if (dataRepr->getInput() == this->Proxy)
              {
                QObject::connect(dataRepr, &pqDataRepresentation::visibilityChanged,
                  [this, self, dataRepr](bool vis) {
                    if (dataRepr->getView())
                    {
                      this->IsVisible = vis;
                      self->updateWidgetVisibility();
                    }
                  });
                QObject::disconnect(this->RepresentationObserver);
                this->IsVisible = dataRepr->isVisible();
                self->updateWidgetVisibility();
              }
            }
          });
      }
    }
  }

  // Reset Representation to nullptr and remove the associated events
  void UnregisterRepresentation()
  {
    this->IsVisible = false;
    QObject::disconnect(this->RepresentationObserver);
  }
};

//-----------------------------------------------------------------------------
class pqInteractivePropertyWidget::pqInternals
{
public:
  vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> WidgetProxy;
  vtkWeakPointer<vtkSMProxy> DataSource;

  // Representation tracker for the filter the widget is attached to.
  SourceTracker Source;
  // When false, HasSourceBeenApplied indicate that the filter the widget
  // is attached to has not been applied yet.
  bool HasSourceBeenApplied = false;
  // Connection to destroy once we have detected the source has been applied
  QMetaObject::Connection ModifiedStateObserver;

  vtkSmartPointer<vtkSMPropertyGroup> SMGroup;
  unsigned long UserEventObserverId = 0;

  pqInternals(pqInteractivePropertyWidget* self, vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup)
    : DataSource(proxy)
    , SMGroup(smgroup)
  {
    auto* smm = pqApplicationCore::instance()->getServerManagerModel();
    auto* source = details::GetTrueSourceInPipeline(proxy);
    if (source == nullptr)
    {
      return;
    }

    if ((this->Source.Proxy = smm->findItem<pqPipelineSource*>(source)))
    {
      this->HasSourceBeenApplied =
        this->Source.Proxy->modifiedState() != pqProxy::ModifiedState::UNINITIALIZED;
      if (!this->HasSourceBeenApplied)
      {
        this->ModifiedStateObserver = QObject::connect(this->Source.Proxy,
          &pqPipelineSource::modifiedStateChanged, [this, self](pqServerManagerModelItem*) {
            if (this->Source.Proxy->modifiedState() == pqProxy::UNMODIFIED)
            {
              this->HasSourceBeenApplied = true;
              QObject::disconnect(this->ModifiedStateObserver);

              // By default first apply always make the object visible but no signal is fired so let
              // set it by hand
              this->Source.IsVisible = true;
              self->updateWidgetVisibility();
            }
          });
      }
    }
  }

  ~pqInternals()
  {
    QObject::disconnect(this->ModifiedStateObserver);
    this->Source.UnregisterRepresentation();
  }
};

//-----------------------------------------------------------------------------
pqInteractivePropertyWidget::pqInteractivePropertyWidget(const char* widget_smgroup,
  const char* widget_smname, vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup,
  QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInteractivePropertyWidget::pqInternals(this, smproxy, smgroup))
{
  assert(widget_smgroup);
  assert(widget_smname);
  assert(smproxy);
  assert(smgroup);

  BEGIN_UNDO_EXCLUDE();

  pqServer* server =
    pqApplicationCore::instance()->getServerManagerModel()->findServer(smproxy->GetSession());

  // Check is server is a Catalyst session. If so, we need to create the widget
  // proxies on the "display-session".
  server = pqLiveInsituVisualizationManager::displaySession(server);

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> aProxy;
  aProxy.TakeReference(pxm->NewProxy(widget_smgroup, widget_smname));
  vtkSMNewWidgetRepresentationProxy* wdgProxy =
    vtkSMNewWidgetRepresentationProxy::SafeDownCast(aProxy);
  if (aProxy == nullptr)
  {
    qCritical("Failed to create proxy for 3D Widget. Aborting for debugging purposes.");
    abort();
  }
  if (wdgProxy == nullptr)
  {
    qCritical() << "Proxy (" << widget_smgroup << ", " << widget_smname
                << ") must be a "
                   "vtkSMNewWidgetRepresentationProxy instance. It however is a '"
                << aProxy->GetClassName() << "'. Aborting for debugging purposes.";
    abort();
  }
  assert(wdgProxy);

  this->Internals->WidgetProxy = wdgProxy;

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(wdgProxy);

  // Setup links between the proxy that the widget is going to be controlling
  wdgProxy->LinkProperties(smproxy, smgroup);

  wdgProxy->UpdateVTKObjects();

  // Marking this as a prototype ensures that the undo/redo system doesn't track
  // changes to the widget.
  wdgProxy->PrototypeOn();

  pqCoreUtilities::connect(wdgProxy, vtkCommand::InteractionEvent, this, SIGNAL(changeAvailable()));
  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::EndInteractionEvent, this, SIGNAL(changeFinished()));

  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::StartInteractionEvent, this, SIGNAL(startInteraction()));
  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::StartInteractionEvent, this, SIGNAL(changeAvailable()));
  pqCoreUtilities::connect(wdgProxy, vtkCommand::InteractionEvent, this, SIGNAL(interaction()));
  pqCoreUtilities::connect(
    wdgProxy, vtkCommand::EndInteractionEvent, this, SIGNAL(endInteraction()));

  if (vtkSMProperty* input = smgroup->GetProperty("Input"))
  {
    this->addPropertyLink(this, "dataSource", SIGNAL(dummySignal()), input);
  }
  else
  {
    this->setDataSource(nullptr);
  }

  // This ensures that when the user changes the Qt widget, we re-render to show
  // the update widget.
  this->connect(&this->links(), SIGNAL(qtWidgetChanged()), SLOT(render()));

  END_UNDO_EXCLUDE();

  this->Internals->UserEventObserverId = smproxy->AddObserver(
    vtkCommand::UserEvent, this, &pqInteractivePropertyWidget::handleUserEvent);
}

//-----------------------------------------------------------------------------
pqInteractivePropertyWidget::~pqInteractivePropertyWidget()
{
  if (this->Internals->UserEventObserverId > 0 && this->proxy())
  {
    this->proxy()->RemoveObserver(this->Internals->UserEventObserverId);
    this->Internals->UserEventObserverId = 0;
  }

  // ensures that the widget proxy is removed from the active view, if any.
  this->setView(nullptr);
}

//-----------------------------------------------------------------------------
vtkSMPropertyGroup* pqInteractivePropertyWidget::propertyGroup() const
{
  return this->Internals->SMGroup;
}

//-----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy* pqInteractivePropertyWidget::widgetProxy() const
{
  return this->Internals->WidgetProxy;
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setView(pqView* pqview)
{
  if (pqview != nullptr && pqview->getServer()->session() != this->widgetProxy()->GetSession())
  {
    pqview = nullptr;
  }

  pqView* rview = qobject_cast<pqRenderViewBase*>(pqview);
  pqView* oldview = this->view();
  if (oldview == rview)
  {
    return;
  }

  if (oldview)
  {
    vtkSMPropertyHelper(oldview->getProxy(), "HiddenRepresentations").Remove(this->widgetProxy());
    oldview->getProxy()->UpdateVTKObjects();

    this->Internals->Source.UnregisterRepresentation();
  }
  this->Superclass::setView(rview);
  if (rview)
  {
    this->Internals->Source.RegisterRepresentationPromise(this, rview);

    vtkSMPropertyHelper(rview->getProxy(), "HiddenRepresentations").Add(this->widgetProxy());
    rview->getProxy()->UpdateVTKObjects();
  }
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::select()
{
  this->Superclass::select();
  this->placeWidget();
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::deselect()
{
  this->Superclass::deselect();
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setWidgetVisible(bool val)
{
  if (this->WidgetVisibility != val)
  {
    SM_SCOPED_TRACE(CallFunction)
      .arg(val ? "Show3DWidgets" : "Hide3DWidgets")
      .arg("proxy", this->proxy())
      .arg("comment", "toggle 3D widget visibility (only when running from the GUI)");

    this->WidgetVisibility = val;
    this->updateWidgetVisibility();
    Q_EMIT this->widgetVisibilityToggled(val);
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::updateWidgetVisibility()
{
  bool dataVis = this->Internals->HasSourceBeenApplied
    ? (this->Internals->Source.Proxy == nullptr) || this->Internals->Source.IsVisible
    : true;
  bool visible = this->isSelected() && this->isWidgetVisible() && this->isPanelVisible() &&
    this->view() && dataVis;
  vtkSMProxy* wdgProxy = this->widgetProxy();
  assert(wdgProxy);

  vtkSMPropertyHelper(wdgProxy, "Visibility", true).Set(visible);
  vtkSMPropertyHelper(wdgProxy, "Enabled", true).Set(visible);
  wdgProxy->UpdateVTKObjects();
  this->render();
  Q_EMIT this->widgetVisibilityUpdated(visible);
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setDataSource(vtkSMProxy* dsource)
{
  this->Internals->DataSource = dsource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqInteractivePropertyWidget::dataSource() const
{
  return this->Internals->DataSource;
}

//-----------------------------------------------------------------------------
vtkBoundingBox pqInteractivePropertyWidget::dataBounds() const
{
  if (vtkSMSourceProxy* dsrc = vtkSMSourceProxy::SafeDownCast(this->dataSource()))
  {
    // FIXME: we need to get the output port number correctly. For now, just use
    // 0.
    vtkPVDataInformation* dataInfo = dsrc->GetDataInformation(0);
    vtkBoundingBox bbox(dataInfo->GetBounds());
    return bbox;
  }
  else
  {
    vtkBoundingBox bbox;
    return bbox;
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::render()
{
  if (pqView* pqview = this->view())
  {
    pqview->render();
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::reset()
{
  this->Superclass::reset();
  this->render();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::handleUserEvent(
  vtkObject* caller, unsigned long eventid, void* calldata)
{
  Q_UNUSED(caller);
  Q_UNUSED(eventid);

  assert(caller == this->proxy());
  assert(eventid == vtkCommand::UserEvent);

  const char* message = reinterpret_cast<const char*>(calldata);
  if (message != nullptr && strcmp("HideWidget", message) == 0)
  {
    this->setWidgetVisible(false);
  }
  else if (message != nullptr && strcmp("ShowWidget", message) == 0)
  {
    this->setWidgetVisible(true);
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setPanelVisible(bool val)
{
  if (this->PanelVisibility != val)
  {
    this->PanelVisibility = val;
    this->updateWidgetVisibility();
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::hideEvent(QHideEvent*)
{
  this->setPanelVisible(false);
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::showEvent(QShowEvent*)
{
  this->setPanelVisible(true);
}
