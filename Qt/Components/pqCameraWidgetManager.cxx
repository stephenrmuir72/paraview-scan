// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCameraWidgetManager.h"

#include "pqApplicationCore.h"
#include "pqCameraWidgetViewLink.h"
#include "pqLinksModel.h"
#include "pqSelectionManager.h"

#include "vtkCamera.h"
#include "vtkCamera3DRepresentation.h"
#include "vtkCamera3DWidget.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"

#include <unordered_map>

namespace
{
const char* MANAGER_NAME = "CameraWidgetManager";

class vtkKeyCallback : public vtkCommand
{
public:
  static vtkKeyCallback* New() { return new vtkKeyCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
    if (!interactor || (interactor->GetKeyCode() != 'p' && interactor->GetKeyCode() != 'P'))
    {
      return;
    }

    if (this->CameraRepresentation)
    {
      pqSelectionManager* selectionManager = qobject_cast<pqSelectionManager*>(
        pqApplicationCore::instance()->manager("SelectionManager"));

      if (selectionManager && selectionManager->hasActiveSelection())
      {
        double center[3];
        selectionManager->selectedDataBounds().GetCenter(center);
        this->CameraRepresentation->GetCamera()->SetFocalPoint(center);
      }
    }
  }

  vtkCamera3DRepresentation* CameraRepresentation = nullptr;

protected:
  vtkKeyCallback() = default;
  ~vtkKeyCallback() override = default;
};
}

struct pqCameraWidgetManager::pqInternals
{
  std::unordered_map<QString, vtkNew<vtkKeyCallback>> KeyCallbacks;
  std::unordered_map<QString, vtkRenderWindowInteractor*> Interactors;
};

//-----------------------------------------------------------------------------
pqCameraWidgetManager::pqCameraWidgetManager(QObject* _parent /*=nullptr*/)
  : QObject(_parent)
  , Internals(new pqInternals)
{
  pqApplicationCore::instance()->registerManager(::MANAGER_NAME, this);

  // When a selection link is added or removed, we need to update the pqCameraWidgetManager
  QObject::connect(pqApplicationCore::instance()->getLinksModel(),
    SIGNAL(linkAdded(int, const QString&)), this, SLOT(onLinkAdded(int, const QString&)));
  QObject::connect(pqApplicationCore::instance()->getLinksModel(), &pqLinksModel::linkRemoved, this,
    &pqCameraWidgetManager::onLinkRemoved);
}

//-----------------------------------------------------------------------------
pqCameraWidgetManager::~pqCameraWidgetManager()
{
  pqApplicationCore::instance()->unRegisterManager(::MANAGER_NAME);
}

//-----------------------------------------------------------------------------
void pqCameraWidgetManager::onLinkAdded(int linkType, const QString& name)
{
  if (linkType != pqLinksModel::CameraWidget)
  {
    return;
  }

  if (this->Internals->KeyCallbacks.find(name) != this->Internals->KeyCallbacks.end())
  {
    this->onLinkRemoved(name);
  }

  pqLinksModel* linksModel = pqApplicationCore::instance()->getLinksModel();
  if (!linksModel->hasCameraWidgetViewLink(name))
  {
    return;
  }

  pqCameraWidgetViewLink* link = linksModel->getCameraWidgetViewLink(name);
  this->Internals->KeyCallbacks[name]->CameraRepresentation = link->getRepresentation();
  this->Internals->Interactors[name] = link->getWidget()->GetInteractor();

  if (this->Internals->Interactors[name])
  {
    this->Internals->Interactors[name]->AddObserver(
      vtkCommand::CharEvent, this->Internals->KeyCallbacks[name]);
  }
}

//-----------------------------------------------------------------------------
void pqCameraWidgetManager::onLinkRemoved(const QString& name)
{
  if (this->Internals->Interactors[name])
  {
    this->Internals->Interactors[name]->RemoveObserver(this->Internals->KeyCallbacks[name]);
  }
  this->Internals->Interactors.erase(name);
  this->Internals->KeyCallbacks.erase(name);
}
