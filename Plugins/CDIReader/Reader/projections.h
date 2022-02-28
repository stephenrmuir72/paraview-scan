/*=========================================================================
 *
 *  Program:   Visualization Toolkit
 *  Module:    projections.h
 *
 *  Copyright (c) 2018 Niklas Roeber, DKRZ Hamburg
 *  All rights reserved.
 *  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
 *
 *     This software is distributed WITHOUT ANY WARRANTY; without even
 *     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *     PURPOSE.  See the above copyright notice for more information.
 *
 *  =========================================================================*/
#ifndef CDI_READER_PROJECTIONS_
#define CDI_READER_PROJECTIONS_

namespace projection
{
enum Projection
{
  SPHERICAL = 0,
  CYLINDRICAL_EQUIDISTANT = 1,
  CASSINI = 2,
  MOLLWEIDE = 3,
  CATALYST = 4,
  SPILHOUSE = 5
};

inline bool isproj(const int p)
{
  return p >= Projection::SPHERICAL && p <= Projection::SPILHOUSE;
}

void get_scaling(const Projection projectionMode, const bool showMultilayerView,
  const double VertLev, const double adjustedLayerThickness, double& sx, double& sy, double& sz);

int cartesianToSpherical(double x, double y, double z, double* rho, double* phi, double* theta);

/**
 * Function to convert spherical coordinates to cartesian, for use in
 * computing points in different layers of multilayer spherical view
 */
int sphericalToCartesian(double rho, double phi, double theta, double* x, double* y, double* z);

/**
 * Function to convert lon/lat coordinates to cartesian
 */
int lLtoXYZ(double lon, double lat, double* x, double* y, double* z, int projectionMode);

/**
 * Compute mollweide projection theta
 * https://en.wikipedia.org/wiki/Mollweide_projection
 */
double iterate_theta(const double lon, const double lat, const double theta);

double get_theta(const double lon, const double lat);

double aasin(double v);

double aacos(double v);

void crossProduct(double vect_A[], double vect_B[], double cross_P[]);

double dotProduct(double vect_A[], double vect_B[]);

void rotateVec(double v[], double r[]);
}; // end of namespace projection

#endif /* CDI_READER_PROJECTIONS_ */
