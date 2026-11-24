// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMRadVizRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVRadVizRepresentation.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMRadVizRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMRadVizRepresentationProxy::vtkSMRadVizRepresentationProxy() = default;

//----------------------------------------------------------------------------
vtkSMRadVizRepresentationProxy::~vtkSMRadVizRepresentationProxy() = default;

//----------------------------------------------------------------------------
void vtkSMRadVizRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMRadVizRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();

  if (auto* repr = vtkPVRadVizRepresentation::SafeDownCast(this->GetClientSideObject()))
  {
    repr->AddObserver(vtkCommand::StartInteractionEvent, this,
      &vtkSMRadVizRepresentationProxy::OnStartInteractionEvent);
    repr->AddObserver(
      vtkCommand::InteractionEvent, this, &vtkSMRadVizRepresentationProxy::OnInteractionEvent);
    repr->AddObserver(vtkCommand::EndInteractionEvent, this,
      &vtkSMRadVizRepresentationProxy::OnEndInteractionEvent);
  }
}

//----------------------------------------------------------------------------
void vtkSMRadVizRepresentationProxy::UpdatePropertiesFromRepresentation()
{
  auto* repr = vtkPVRadVizRepresentation::SafeDownCast(this->GetClientSideObject());
  if (!repr)
  {
    return;
  }

  vtkSMPropertyHelper(this, "VerticesRotation", /*quiet*/ true).Set(repr->GetVerticesRotation());
  vtkSMPropertyHelper(this, "LensCenter", /*quiet*/ true).Set(repr->GetLensCenter(), 2);
  vtkSMPropertyHelper(this, "LensRadius", /*quiet*/ true).Set(repr->GetLensRadius());

  // values from a drag in the plot: push them to the other processes without
  // marking this proxy modified, as CopyAxisRangesFromChart() does.
  int prev = this->InMarkModified;
  this->InMarkModified = 1;
  this->UpdateVTKObjects();
  this->InMarkModified = prev;
}

//----------------------------------------------------------------------------
void vtkSMRadVizRepresentationProxy::OnStartInteractionEvent()
{
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
}

//----------------------------------------------------------------------------
void vtkSMRadVizRepresentationProxy::OnInteractionEvent()
{
  this->InvokeEvent(vtkCommand::InteractionEvent);
}

//----------------------------------------------------------------------------
void vtkSMRadVizRepresentationProxy::OnEndInteractionEvent()
{
  this->UpdatePropertiesFromRepresentation();
  this->InvokeEvent(vtkCommand::EndInteractionEvent);
}
