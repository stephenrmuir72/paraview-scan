// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef Adaptors_NPICAdaptor_h
#define Adaptors_NPICAdaptor_h

#include "vtkPVAdaptorsNPICModule.h"
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_6_3_0

// This code is meant to be used as an API for C simulation
// codes. To use with C codes, include this header file. Call the
// 'extern "C"' functions as named below for both Fortran and C.
// C code should include this header file to get the properly
// mangled function names.

#ifdef __cplusplus
extern "C"
{
#endif

  PARAVIEW_DEPRECATED_IN_6_3_0("The NPIC adaptor is unused.")
  void VTKPVADAPTORSNPIC_EXPORT createstructuredgrid_(
    int* myid, int* xdim, int* ystart, int* ystop, double* xspc, double* yspc);

  PARAVIEW_DEPRECATED_IN_6_3_0("The NPIC adaptor is unused.")
  void VTKPVADAPTORSNPIC_EXPORT add_scalar_(char* fname, int* len, double* data, int* size);

  PARAVIEW_DEPRECATED_IN_6_3_0("The NPIC adaptor is unused.")
  void VTKPVADAPTORSNPIC_EXPORT add_vector_(
    char* fname, int* len, double* data0, double* data1, double* data2, int* size);

  PARAVIEW_DEPRECATED_IN_6_3_0("The NPIC adaptor is unused.")
  void VTKPVADAPTORSNPIC_EXPORT add_pressure_(int* index, double* data, int* size);

#ifdef __cplusplus
}
#endif
#endif
