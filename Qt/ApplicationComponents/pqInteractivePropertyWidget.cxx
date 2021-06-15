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
#include "pqLiveInsituVisualizationManager.h"
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
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QtDebug>

#include <cassert>

//-----------------------------------------------------------------------------
struct pqInteractivePropertyWidget::SourceTracker
{
  vtkSMProxy* Proxy = nullptr;
  vtkSMRepresentationProxy* Representation = nullptr;
  int Visibility = 0;

  // Try to find the representation of our current Proxy in the view. If found
  // we link its visibility property to trigger an update of the widget visibility.
  void RegisterRepresentation(
    pqInteractivePropertyWidget* self, vtkSMViewProxy* view, const char* prop)
  {
    if (vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(this->Proxy))
    {
      // XXX: for now we only consider port 0
      // This is coherent with the pqInteractivePropertyWidget::dataBounds method
      vtkSMRepresentationProxy* repr = view ? view->FindRepresentation(source, 0) : nullptr;

      // Links its visibility property to trigger a render
      if (repr)
      {
        if (repr != this->Representation)
        {
          this->Representation = repr;
          if (vtkSMProperty* visibility = repr->GetProperty("Visibility"))
          {
            self->addPropertyLink(self, prop, SIGNAL(dummySignal()), visibility);
            int visValue = 0;
            vtkSMPropertyHelper(visibility, true).Get(&visValue, 1);
            self->setProperty(prop, visValue);
          }
        }
      }
      else
      {
        self->setProperty(prop, 0);
      }
    }
  }

  // Reset Representation to nullptr and remove the associated events
  void UnregisterRepresentation(pqInteractivePropertyWidget* self, const char* prop)
  {
    if (this->Representation)
    {
      if (vtkSMProperty* visibility = this->Representation->GetProperty("Visibility"))
      {
        self->removePropertyLink(self, prop, SIGNAL(dummySignal()), visibility);
      }
      this->Representation = nullptr;
    }
  }
};

//-----------------------------------------------------------------------------
class pqInteractivePropertyWidget::pqInternals
{
public:
  vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> WidgetProxy;
  // This pointer is the proxy of the property and not the filter itself
  vtkWeakPointer<vtkSMProxy> DataSource;
  vtkSmartPointer<vtkSMPropertyGroup> SMGroup;
  bool WidgetVisibility;
  bool ViewRepresentationSynchronized = false;
  unsigned long UserEventObserverId;

  // When false, HasSourceBeenApplied indicate that the filter the widget
  // is attached to has not been applied yet.
  bool HasSourceBeenApplied = false;
  // Connection to destroy once we have detected the source has been applied
  QMetaObject::Connection ApplyConnectionObserver;
  // Tracker for the filter the widget is attached to. Its visibility is used when
  // HasSourceBeenApplied == true
  SourceTracker Source;
  // Tracker for the parent filter of the this->Source filter. Its visibility is
  // used when HasSourceBeenApplied == false
  SourceTracker Parent;

  pqInternals()
    : WidgetVisibility(false)
    , UserEventObserverId(0)
  {
  }
};

//-----------------------------------------------------------------------------
pqInteractivePropertyWidget::pqInteractivePropertyWidget(const char* widget_smgroup,
  const char* widget_smname, vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup,
  QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInteractivePropertyWidget::pqInternals())
{
  assert(widget_smgroup);
  assert(widget_smname);
  assert(smproxy);
  assert(smgroup);

  BEGIN_UNDO_EXCLUDE();

  pqInternals& internals = (*this->Internals);
  internals.SMGroup = smgroup;

  // Initialize related proxies. For example with a `Sphere -> Transform` pipeline,
  // proxies will be :
  // - this->proxy() : property the widget is linked to
  // - internals.Source.Proxy : the actual filter (ie Transform)
  // - internals.Parent.Proxy : the parent filter (ie Sphere)
  internals.Source.Proxy = this->proxy()->GetConsumerProxy(0);
  if (this->proxy()->GetNumberOfProducers() > 0)
  {
    internals.Parent.Proxy = this->proxy()->GetProducerProxy(0);
  }

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

  internals.WidgetProxy = wdgProxy;

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

  internals.ApplyConnectionObserver =
    QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
      &pqServerManagerModel::dataUpdated, [this, &internals](pqPipelineSource* source) {
        if (!internals.HasSourceBeenApplied && source->getProxy() == internals.Source.Proxy)
        {
          internals.HasSourceBeenApplied = true;
          this->updateWidgetVisibility();

          // Unregister this connection once we know the filter has been applied
          QObject::disconnect(internals.ApplyConnectionObserver);
        }
      });

  // This ensures that when the user changes the Qt widget, we re-render to show
  // the update widget.
  this->connect(&this->links(), SIGNAL(qtWidgetChanged()), SLOT(render()));

  END_UNDO_EXCLUDE();

  internals.UserEventObserverId = smproxy->AddObserver(
    vtkCommand::UserEvent, this, &pqInteractivePropertyWidget::handleUserEvent);
}

//-----------------------------------------------------------------------------
pqInteractivePropertyWidget::~pqInteractivePropertyWidget()
{
  pqInternals& internals = (*this->Internals);
  if (internals.UserEventObserverId > 0 && this->proxy())
  {
    this->proxy()->RemoveObserver(internals.UserEventObserverId);
    internals.UserEventObserverId = 0;
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

  this->Internals->ViewRepresentationSynchronized = false;

  if (oldview)
  {
    QObject::disconnect(oldview, &pqView::representationAdded, this,
      &pqInteractivePropertyWidget::onRepresentationAdded);

    vtkSMPropertyHelper(oldview->getProxy(), "HiddenRepresentations").Remove(this->widgetProxy());
    oldview->getProxy()->UpdateVTKObjects();

    this->Internals->Source.UnregisterRepresentation(this, "sourceVisibility");
    this->Internals->Parent.UnregisterRepresentation(this, "parentVisibility");
  }
  this->Superclass::setView(rview);
  if (rview)
  {
    QObject::connect(rview, &pqView::representationAdded, this,
      &pqInteractivePropertyWidget::onRepresentationAdded);

    vtkSMPropertyHelper(rview->getProxy(), "HiddenRepresentations").Add(this->widgetProxy());
    rview->getProxy()->UpdateVTKObjects();

    this->setupCurrentDataRepresentation();
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
bool pqInteractivePropertyWidget::isWidgetVisible() const
{
  return this->Internals->WidgetVisibility;
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setWidgetVisible(bool val)
{
  pqInternals& internals = (*this->Internals);
  if (internals.WidgetVisibility != val)
  {
    SM_SCOPED_TRACE(CallFunction)
      .arg(val ? "Show3DWidgets" : "Hide3DWidgets")
      .arg("proxy", this->proxy())
      .arg("comment", "toggle 3D widget visibility (only when running from the GUI)");

    internals.WidgetVisibility = val;
    this->updateWidgetVisibility();
    Q_EMIT this->widgetVisibilityToggled(val);
  }
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::updateWidgetVisibility()
{
  bool dataVisible = this->Internals->HasSourceBeenApplied ? this->Internals->Source.Visibility
                                                           : this->Internals->Parent.Visibility;
  bool visible = this->isSelected() && this->isWidgetVisible() && this->view() && dataVisible;
  vtkSMProxy* wdgProxy = this->widgetProxy();
  assert(wdgProxy);

  vtkSMPropertyHelper(wdgProxy, "Visibility", true).Set(visible);
  vtkSMPropertyHelper(wdgProxy, "Enabled", true).Set(visible);
  wdgProxy->UpdateVTKObjects();
  this->render();
  Q_EMIT this->widgetVisibilityUpdated(visible);
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::onRepresentationAdded(pqRepresentation* representation)
{
  if (this->Internals->ViewRepresentationSynchronized)
  {
    return;
  }

  this->setupCurrentDataRepresentation();
  if (this->Internals->Source.Representation == representation->getProxy())
  {
    this->Internals->ViewRepresentationSynchronized = true;
  }

  this->updateWidgetVisibility();
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
void pqInteractivePropertyWidget::setSourceVisibility(int vis)
{
  this->Internals->Source.Visibility = vis;
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::setParentVisibility(int vis)
{
  this->Internals->Parent.Visibility = vis;
  this->updateWidgetVisibility();
}

//-----------------------------------------------------------------------------
int pqInteractivePropertyWidget::sourceVisibility() const
{
  return this->Internals->Source.Visibility;
}

//-----------------------------------------------------------------------------
int pqInteractivePropertyWidget::parentVisibility() const
{
  return this->Internals->Parent.Visibility;
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
void pqInteractivePropertyWidget::setupCurrentDataRepresentation()
{
  if (this->Internals->ViewRepresentationSynchronized)
  {
    return;
  }

  vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(this->view()->getProxy());
  this->Internals->Source.RegisterRepresentation(this, view, "sourceVisibility");
  this->Internals->Parent.RegisterRepresentation(this, view, "parentVisibility");
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::hideEvent(QHideEvent*)
{
  this->VisibleState = vtkSMPropertyHelper(this->widgetProxy(), "Visibility").GetAsInt() != 0;
  this->setWidgetVisible(false);
}

//-----------------------------------------------------------------------------
void pqInteractivePropertyWidget::showEvent(QShowEvent*)
{
  this->setWidgetVisible(this->VisibleState);
}
