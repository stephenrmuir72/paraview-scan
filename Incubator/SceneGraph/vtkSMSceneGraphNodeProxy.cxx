// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMSceneGraphNodeProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMSceneGraphNodeProxy);

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy::vtkSMSceneGraphNodeProxy() = default;

//----------------------------------------------------------------------------
vtkSMSceneGraphNodeProxy::~vtkSMSceneGraphNodeProxy() = default;

//----------------------------------------------------------------------------
void vtkSMSceneGraphNodeProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
