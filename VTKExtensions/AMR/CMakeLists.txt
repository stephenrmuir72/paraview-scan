# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAMRConnectivity
  vtkAMRDualClip
  vtkAMRDualContour
  vtkAMRDualGridHelper
  vtkAMRFragmentIntegration
  vtkAMRFragmentsFilter
  vtkPVAMRDualClip
  vtkPVAMRDualContour
  vtkPVAMRFragmentIntegration)
 
vtk_module_add_module(ParaView::VTKExtensionsAMR
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  "Resources/amr_filters.xml")

# FIXME vtkPVAMRDualContour.cxx:18:14: error: redefinition of ‘const double PV_AMR_SURFACE_VALUE_UNSIGNED_CHAR
set_property(SOURCE vtkPVAMRDualContour.cxx vtkAMRDualClip.cxx
  PROPERTY
    SKIP_UNITY_BUILD_INCLUSION ON)
