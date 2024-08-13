// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVDisplaySizedImplicitPlaneRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVDisplaySizedImplicitPlaneRepresentation);

//----------------------------------------------------------------------------
vtkPVDisplaySizedImplicitPlaneRepresentation::vtkPVDisplaySizedImplicitPlaneRepresentation()
{
  double opacity = 0.25;
  this->GetPlaneProperty()->SetOpacity(opacity);
  this->GetSelectedPlaneProperty()->SetOpacity(opacity);
}

//----------------------------------------------------------------------------
vtkPVDisplaySizedImplicitPlaneRepresentation::~vtkPVDisplaySizedImplicitPlaneRepresentation() =
  default;

//----------------------------------------------------------------------------
void vtkPVDisplaySizedImplicitPlaneRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
