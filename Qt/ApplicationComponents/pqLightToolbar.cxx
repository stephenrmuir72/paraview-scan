/*=========================================================================

   Program: ParaView
   Module:    pqLightToolbar.cxx

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
#include "pqLightToolbar.h"
#include "ui_pqLightToolbar.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqRenderViewSelectionReaction.h"
#include "pqView.h"
#include "vtkPVRenderView.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMTrace.h"

#include <cmath>

class pqLightToolbar::pqInternals : public Ui::pqLightToolbar
{
public:
  pqView* View = nullptr;
  pqPropertyLinks Links;
};

//-----------------------------------------------------------------------------
void pqLightToolbar::constructor()
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  this->Internals->Links.setUseUncheckedProperties(false);
  this->Internals->Links.setAutoUpdateVTKObjects(true);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setView(pqView*)));

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(updateEnabledState()), Qt::QueuedConnection);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnabledState()));

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(representationChanged(pqRepresentation*)),
    this, SLOT(updateEnabledState()));

  QObject::connect(
    this->Internals->actionActiveLightKit, SIGNAL(toggled(bool)), this, SLOT(toggleLightKit(bool)));

  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
pqLightToolbar::~pqLightToolbar()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqLightToolbar::setView(pqView* view)
{
  if (this->Internals->View == view)
  {
    return;
  }

  this->Internals->View = view;
  this->Internals->Links.removeAllPropertyLinks();

  if (!(view && view->getProxy()->GetProperty("UseLight")))
  {
    return;
  }

  this->Internals->Links.addPropertyLink(this->Internals->actionActiveLightKit, "checked",
    SIGNAL(toggled(bool)), view->getProxy(), view->getProxy()->GetProperty("UseLight"));
}

//-----------------------------------------------------------------------------
void pqLightToolbar::updateEnabledState()
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  this->Internals->actionActiveLightKit->setEnabled(renderView != nullptr);
}

//-----------------------------------------------------------------------------
void pqLightToolbar::toggleLightKit(bool useLight)
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!renderView)
  {
    return;
  }

  vtkSMRenderViewProxy* proxy = renderView->getRenderViewProxy();
  if (!proxy)
  {
    return;
  }

  vtkPVRenderView* pvRenderView = vtkPVRenderView::SafeDownCast(proxy->GetClientSideView());
  if (!pvRenderView)
  {
    return;
  }

  pvRenderView->SetUseLightKit(useLight);
  renderView->render();
}
