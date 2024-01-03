// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef FEDATASTRUCTURES_HEADER
#define FEDATASTRUCTURES_HEADER

#include <cstddef>
#include <cstdint>
#include <vector>

enum SpaceTypes
{
  Device,
  Unified
};

class Grid
{
public:
  Grid();
  ~Grid();
  void Initialize(const unsigned int numPoints[3], const float spacing[3], SpaceTypes& memSpace);
  size_t GetNumberOfPoints();
  size_t GetNumberOfCells();
  float* GetPointsArray();
  float* GetPoint(size_t pointId);
  int* GetCellPoints(size_t cellId);
  inline SpaceTypes GetMemorySpaceInUse() const noexcept { return this->MemorySpace; }

private:
  float* Points = nullptr;
  std::size_t NumPoints = 0;
  int* Cells = nullptr;
  std::size_t NumCells = 0;
  SpaceTypes MemorySpace;
};

class Attributes
{
  // A class for generating and storing point and cell fields.
  // Velocity is stored at the points and pressure is stored
  // for the cells. The current velocity profile is for a
  // shearing flow with U(y,t) = y*t, V = 0 and W = 0.
  // Pressure is constant through the domain.
public:
  Attributes();
  ~Attributes();
  void Initialize(Grid* grid);
  void UpdateFields(double time);
  float* GetVelocityArray();
  float* GetPressureArray();

private:
  float* Velocity = nullptr;
  float* Pressure = nullptr;
  Grid* GridPtr = nullptr;
};
#endif
