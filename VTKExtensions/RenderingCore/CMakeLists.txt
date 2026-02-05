# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVAxesActor
  vtkPVCenterAxesActor
  vtkPVDefaultPass
  vtkPVGridAxes3DActor
  vtkPVLODActor
  vtkPVRendererCuller)

vtk_module_add_module(ParaView::VTKExtensionsRenderingCore
  CLASSES ${classes})
