/*=========================================================================

  Program:   ParaView
  Module:    PagosaAdaptor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @brief Simulation code.
 *
 * Pagosa is a simulation code. It is a closed source code. A copy of the Physics
 * Manual is at http://permalink.lanl.gov/object/tr?what=info:lanl-repo/lareport/LA-14425-M
 */

#ifndef vtkCommandOptions_h
#define vtkCommandOptions_h

/**
 * Define the data structures to hold the in situ output VTK data
 * vtkNonOverlappingAMR is required for using the MaterialInterface filter
 * vtkUnstructuredGrid will hold the data currently written to .cosmo files.
 */
extern "C" void setcoprocessorgeometry_(int* mx, int* my, int* mz, double* x0, double* y0,
  double* z0, double* dx, double* dy, double* dz, unsigned int* my_id, const int* tot_pes);

/**
 * Set a field in the first grid of nonoverlapping AMR
 */
extern "C" void setcoprocessorfield_(char* fname, int* len, int* mx, int* my, int* mz,
  unsigned int* my_id, float* data, bool* down_convert);

/**
 * Initialize unstructured grid for markers and allocate size.
 */
extern "C" void setmarkergeometry_(int* nvp, int* my_id);

/**
 * Add a field to the unstructured grid of markers
 *
 * @param numberOfParticles Particles in added field
 * @param xloc Location
 * @param yloc Location
 * @param xloc Location
 */
extern "C" void addmarkergeometry_(int* numberOfParticles, float* xloc, float* yloc, float* zloc);

/*
 * Set a scalar field in the unstructured grid of markers
 *
 * @param fname Name of data
 * @param len Length of data name
 * @param numberOfParticles Particles in field
 * @param data Data by particle
 */
extern "C" void addmarkerscalarfield_(char* fname, int* len, int* numberOfParticles, float* data);

/**
 * Set a vector field in the unstructured grid of markers
 *
 * @param fname Name of data
 * @param len Length of data name
 * @param numberOfParticles Particles in field
 * @param data0 Data by particle
 * @param data1 Data by particle
 * @param data2 Data by particle
 */
extern "C" void addmarkervectorfield_(
  char* fname, int* len, int* numberOfParticles, float* data0, float* data1, float* data2);

/**
 * Set a 6 element tensor field in the unstructured grid of markers
 *
 * @param fname Name of data
 * @param len Length of data name
 * @param numberOfParticles Particles in field
 * @param data0 Data by particle
 * @param data1 Data by particle
 * @param data2 Data by particle
 * @param data3 Data by particle
 * @param data4 Data by particle
 * @param data5 Data by particle
 */
extern "C" void addmarkertensorfield_(char* fname, int* len, int* numberOfParticles, float* data0,
  float* data1, float* data2, float* data3, float* data4, float* data5);

#endif
