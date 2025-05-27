// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// For PARAVIEW_DEPRECATED_IN_5_14_0()
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "vtkPVPlane.h"

#include "vtkObjectFactory.h"

#include <cmath>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPlane);

//----------------------------------------------------------------------------
vtkPVPlane::vtkPVPlane() = default;

//----------------------------------------------------------------------------
vtkPVPlane::~vtkPVPlane() = default;
