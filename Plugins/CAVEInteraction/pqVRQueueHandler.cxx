// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVRQueueHandler.h"

#include "vtkPVVRConfig.h"

#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMVRInteractorStyleProxy.h"
#include "vtkTimerLog.h"
#include "vtkVRInteractorStyleFactory.h"
#include "vtkVRQueue.h"
#include "vtkWeakPointer.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqView.h"

#if PARAVIEW_PLUGIN_ENABLE_SceneGraph
#include "vtkCommand.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSceneGraphProxy.h"
#include "vtkSMSessionProxyManager.h"
#endif

#include <QList>
#include <QTimer>
#include <QtDebug>

#include <cassert>
#include <cstring>
#include <vector>

namespace
{
vtkSMRenderViewProxy* ActiveRenderView()
{
  pqView* view = pqActiveObjects::instance().activeView();
  return view ? vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy()) : nullptr;
}
}

class pqVRQueueHandler::pqInternals
{
public:
  vtkNew<vtkCollection> Styles;
  vtkWeakPointer<vtkVRQueue> Queue;
  QTimer Timer;

#if PARAVIEW_PLUGIN_ENABLE_SceneGraph
  // Keep a list of all the scenegraphs, and watch proxy manager registration
  // events so we can mark the list stale when appropriate. If the event loop
  // code sees the iist is stale, it rebuilds the list.
  vtkWeakPointer<vtkSMSessionProxyManager> ProxyManager;
  unsigned long RegisterTag = 0;
  unsigned long UnRegisterTag = 0;
  bool SceneGraphsStale = true;
  std::vector<vtkWeakPointer<vtkSMSceneGraphProxy>> SceneGraphs;
  vtkNew<vtkCollection> TargetViews; // scratch, reused every frame

  void onRegistration(vtkObject*, unsigned long, void* callData)
  {
    auto* info = static_cast<vtkSMProxyManager::RegisteredProxyInformation*>(callData);
    if (info && (info->Type & vtkSMProxyManager::RegisteredProxyInformation::PROXY) &&
      info->GroupName &&
      std::strcmp(info->GroupName, vtkSMSceneGraphProxy::GetSceneGraphGroup()) == 0)
    {
      this->SceneGraphsStale = true;
    }
  }

  void watch(vtkSMSessionProxyManager* pxm)
  {
    if (this->ProxyManager)
    {
      this->ProxyManager->RemoveObserver(this->RegisterTag);
      this->ProxyManager->RemoveObserver(this->UnRegisterTag);
    }
    this->ProxyManager = pxm;
    this->RegisterTag = this->UnRegisterTag = 0;
    if (pxm)
    {
      this->RegisterTag =
        pxm->AddObserver(vtkCommand::RegisterEvent, this, &pqInternals::onRegistration);
      this->UnRegisterTag =
        pxm->AddObserver(vtkCommand::UnRegisterEvent, this, &pqInternals::onRegistration);
    }
    this->SceneGraphsStale = true;
  }

  void refreshSceneGraphs()
  {
    this->SceneGraphs.clear();
    if (this->ProxyManager)
    {
      vtkNew<vtkCollection> graphs;
      vtkSMSceneGraphProxy::GetSceneGraphs(this->ProxyManager, graphs);
      for (int cc = 0; cc < graphs->GetNumberOfItems(); ++cc)
      {
        this->SceneGraphs.emplace_back(
          vtkSMSceneGraphProxy::SafeDownCast(graphs->GetItemAsObject(cc)));
      }
    }
    this->SceneGraphsStale = false;
  }

  /// Apply every scenegraph that targets `view`, so the styles' edits to
  /// node properties are applied to the representations before the frame
  /// is rendered.
  void applySceneGraphs(vtkSMRenderViewProxy* view)
  {
    if (this->SceneGraphsStale)
    {
      this->refreshSceneGraphs();
    }
    if (!view)
    {
      return;
    }
    // Every graph targeting the given view is applied every frame, while graphs
    // targeting other views are skipped.
    for (auto& weak : this->SceneGraphs)
    {
      vtkSMSceneGraphProxy* graph = weak;
      if (!graph)
      {
        continue;
      }
      graph->GetTargetViews(this->TargetViews);
      for (int cc = 0; cc < this->TargetViews->GetNumberOfItems(); ++cc)
      {
        if (this->TargetViews->GetItemAsObject(cc) == view)
        {
          graph->TraverseAndApply();
          break;
        }
      }
    }
  }

  ~pqInternals() { this->watch(nullptr); }
#endif
};

//----------------------------------------------------------------------------
QPointer<pqVRQueueHandler> pqVRQueueHandler::Instance;

//----------------------------------------------------------------------------
pqVRQueueHandler* pqVRQueueHandler::instance()
{
  return pqVRQueueHandler::Instance;
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::setInstance(pqVRQueueHandler* inst)
{
  pqVRQueueHandler::Instance = inst;
}

//----------------------------------------------------------------------------
pqVRQueueHandler::pqVRQueueHandler(vtkVRQueue* queue, QObject* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();
  this->Internals->Queue = queue;
  this->Internals->Timer.setInterval(1);
  this->Internals->Timer.setSingleShot(true);
  QObject::connect(&this->Internals->Timer, SIGNAL(timeout()), this, SLOT(processEvents()));

  QObject::connect(pqApplicationCore::instance(),
    SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)), this,
    SLOT(configureStyles(vtkPVXMLElement*, vtkSMProxyLocator*)));

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::serverChanged, this,
    &pqVRQueueHandler::onServerChanged);
  this->onServerChanged(pqActiveObjects::instance().activeServer());
  QObject::connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(saveStylesConfiguration(vtkPVXMLElement*)));
}

//----------------------------------------------------------------------------
pqVRQueueHandler::~pqVRQueueHandler()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::add(vtkSMVRInteractorStyleProxy* style)
{
  if (!this->Internals->Styles->IsItemPresent(style))
  {
    this->Internals->Styles->AddItem(style);
    Q_EMIT stylesChanged();
    return;
  }
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::remove(vtkSMVRInteractorStyleProxy* style)
{
  this->Internals->Styles->RemoveItem(style);
  Q_EMIT stylesChanged();
}

//----------------------------------------------------------------------public
void pqVRQueueHandler::clear()
{
  if (this->Internals->Styles->GetNumberOfItems() != 0)
  {
    this->Internals->Styles->RemoveAllItems();
    Q_EMIT stylesChanged();
  }
}

//----------------------------------------------------------------------public
QList<vtkSMVRInteractorStyleProxy*> pqVRQueueHandler::styles()
{
  vtkSMVRInteractorStyleProxy* style;
  QList<vtkSMVRInteractorStyleProxy*> result;
  for (this->Internals->Styles->InitTraversal();
       (style = vtkSMVRInteractorStyleProxy::SafeDownCast(
          this->Internals->Styles->GetNextItemAsObject()));)
  {
    result << style;
  }

  return result;
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::start()
{
  this->Internals->Timer.start();
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::stop()
{
  this->Internals->Timer.stop();
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::processEvents()
{
  assert(this->Internals->Queue != nullptr);
  std::queue<vtkVREvent> events;
  this->Internals->Queue->TryDequeue(events);

  // Loop through the event queue and pass events to InteractorStyles
  while (!events.empty())
  {
    vtkVREvent event = events.front(); /* get first event on the queue */
    events.pop();
    vtkSMVRInteractorStyleProxy* style;
    for (this->Internals->Styles->InitTraversal();
         (style = vtkSMVRInteractorStyleProxy::SafeDownCast(
            this->Internals->Styles->GetNextItemAsObject()));)
    {
      style->HandleEvent(event);
    }
  }

  // There should be an explicit update for each handler. Otherwise the server
  // side updates will not happen
  vtkSMVRInteractorStyleProxy* style;
  for (this->Internals->Styles->InitTraversal();
       (style = vtkSMVRInteractorStyleProxy::SafeDownCast(
          this->Internals->Styles->GetNextItemAsObject()));)
  {
    style->Update();
  }

#if PARAVIEW_PLUGIN_ENABLE_SceneGraph
  // The styles may have set properties of nodes (e.g transform). Apply
  // those to the representations before the frame is drawn.
  this->Internals->applySceneGraphs(ActiveRenderView());
#endif

  this->render();

  // since timer is single-shot we start it again.
  this->Internals->Timer.start();
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::onServerChanged(pqServer* server)
{
#if PARAVIEW_PLUGIN_ENABLE_SceneGraph
  this->Internals->watch(server ? server->proxyManager() : nullptr);
#else
  (void)server;
#endif
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::render()
{
  if (vtkSMRenderViewProxy* proxy = ActiveRenderView())
  {
    proxy->StillRender();
  }
}

//----------------------------------------------------------------------------
/* Sample PVVR <VRInteractorStyles> configuration:            */
/*                                                            */
/*   <VRInteractorStyles>                                     */
/*      <Style class="vtkVRStyleGrabNRotateWorld">            */
/*        <Button name="wiimote.A"/>                          */
/*        <Tracker name="wiimote.tracker"/>                   */
/*      </Style>                                              */
/*      <Style class="vtkVRWandTrackingStyle">                */
/*        <Event name="wiitracker.hand" type = "tracker"/>    */
/*      </Style>                                              */
/*   </VRInteractorStyles>                                    */
/*                                                            */
void pqVRQueueHandler::configureStyles(vtkPVXMLElement* xml, vtkSMProxyLocator* locator)
{
  if (!xml)
  {
    return;
  }

  if (xml->GetName() && strcmp(xml->GetName(), "VRInteractorStyles") == 0)
  {
    this->clear();
    vtkVRInteractorStyleFactory* factory = vtkVRInteractorStyleFactory::GetInstance();
    for (unsigned cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
    {
      vtkPVXMLElement* child = xml->GetNestedElement(cc);
      if (child && child->GetName() && strcmp(child->GetName(), "Style") == 0)
      {
        const char* class_name = child->GetAttributeOrEmpty("class");
        vtkSMVRInteractorStyleProxy* style = factory->NewInteractorStyleFromClassName(class_name);
        if (style)
        {
          if (style->Configure(child, locator))
          {
            this->add(style);
          }
          style->Delete();
        }
        else
        {
          qWarning() << "Unknown interactor style: \"" << class_name << "\"";
        }
      }
    }
  }
  else
  {
    this->configureStyles(xml->FindNestedElementByName("VRInteractorStyles"), locator);
  }

  Q_EMIT stylesChanged();
}

//----------------------------------------------------------------------------
void pqVRQueueHandler::saveStylesConfiguration(vtkPVXMLElement* root)
{
  assert(root != nullptr);

  vtkPVXMLElement* tempParent = vtkPVXMLElement::New();
  tempParent->SetName("VRInteractorStyles");
  vtkSMVRInteractorStyleProxy* style;
  for (this->Internals->Styles->InitTraversal();
       (style = vtkSMVRInteractorStyleProxy::SafeDownCast(
          this->Internals->Styles->GetNextItemAsObject()));)
  {
    vtkPVXMLElement* child = style->SaveConfiguration();
    if (child)
    {
      tempParent->AddNestedElement(child);
      child->Delete();
    }
  }
  root->AddNestedElement(tempParent);
  tempParent->Delete();
}
