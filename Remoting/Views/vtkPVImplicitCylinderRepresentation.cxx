// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVImplicitCylinderRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVImplicitCylinderRepresentation);

//----------------------------------------------------------------------------
vtkPVImplicitCylinderRepresentation::vtkPVImplicitCylinderRepresentation()
{
  double opacity = 0.25;
  this->GetCylinderProperty()->SetOpacity(opacity);
  this->GetSelectedCylinderProperty()->SetOpacity(opacity);
}

//----------------------------------------------------------------------------
vtkPVImplicitCylinderRepresentation::~vtkPVImplicitCylinderRepresentation() = default;

//----------------------------------------------------------------------------
void vtkPVImplicitCylinderRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
