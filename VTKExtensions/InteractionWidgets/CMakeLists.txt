# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVAxesWidget)

vtk_module_add_module(ParaView::VTKExtensionsInteractionWidgets
  CLASSES ${classes})
