// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVImplicitConeRepresentation.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVImplicitConeRepresentation);

//----------------------------------------------------------------------------
vtkPVImplicitConeRepresentation::vtkPVImplicitConeRepresentation()
{
  vtkMultiProcessController* ctrl = nullptr;
  ctrl = vtkMultiProcessController::GetGlobalController();
  double opacity = 1;
  if (ctrl == nullptr || ctrl->GetNumberOfProcesses() == 1)
  {
    opacity = 0.25;
  }

  this->GetConeProperty()->SetOpacity(opacity);
  this->GetSelectedConeProperty()->SetOpacity(opacity);
}
