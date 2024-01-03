// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "FEDataStructures.h"

#include <iostream>
#include <iterator>
#include <mpi.h>

namespace
{
__global__ void GeneratePointCoordinates(
  unsigned int offsetX, unsigned int dimY, unsigned int dimZ, float* points, const int numPoints)
{
  const int threadId = blockIdx.x * blockDim.x + threadIdx.x;
  if (threadId >= numPoints)
  {
    return;
  }
  // k increases fastest, i increases slowest.
  int i = threadId / (dimY * dimZ);
  int temp = threadId - (i * dimY * dimZ);
  int j = temp / dimZ;
  int k = temp % dimZ;
  int coordId = threadId * 3;
  points[coordId++] = i + offsetX;
  points[coordId++] = j;
  points[coordId] = k;
}

__global__ void GenerateHexahedrons(
  unsigned int dimY, unsigned int dimZ, int* connectivity, const int numCells)
{
  const int threadId = blockIdx.x * blockDim.x + threadIdx.x;
  if (threadId >= numCells)
  {
    return;
  }
  // k increases fastest, i increases slowest.
  int i = threadId / ((dimY - 1) * (dimZ - 1));
  int temp = threadId - (i * (dimY - 1) * (dimZ - 1));
  int j = temp / (dimZ - 1);
  int k = temp % (dimZ - 1);
  int* outPtr = connectivity + threadId * 8;
  outPtr[0] = i * dimY * dimZ + j * dimZ + k;
  outPtr[1] = (i + 1) * dimY * dimZ + j * dimZ + k;
  outPtr[2] = (i + 1) * dimY * dimZ + (j + 1) * dimZ + k;
  outPtr[3] = i * dimY * dimZ + (j + 1) * dimZ + k;
  outPtr[4] = i * dimY * dimZ + j * dimZ + k + 1;
  outPtr[5] = (i + 1) * dimY * dimZ + j * dimZ + k + 1;
  outPtr[6] = (i + 1) * dimY * dimZ + (j + 1) * dimZ + k + 1;
  outPtr[7] = i * dimY * dimZ + (j + 1) * dimZ + k + 1;
}
} // namespace

Grid::Grid() = default;

Grid::~Grid()
{
  if (this->Points != nullptr)
  {
    cudaFree(this->Points);
  }
  if (this->Cells != nullptr)
  {
    cudaFree(this->Cells);
  }
}

void Grid::Initialize(const unsigned int dims[3], const float spacing[3], SpaceTypes& memSpace)
{
  this->MemorySpace = memSpace;
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    std::cerr << "Must have a non-zero amount of points in each direction.\n";
  }
  // in parallel, we do a simple partitioning in the x-direction.
  int mpiSize = 1;
  int mpiRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

  unsigned int startXPoint = mpiRank * dims[0] / mpiSize;
  unsigned int endXPoint = (mpiRank + 1) * dims[0] / mpiSize;
  if (mpiSize != mpiRank + 1)
  {
    endXPoint++;
  }
  unsigned int adjustedDim0 = endXPoint - startXPoint;
  this->NumPoints = adjustedDim0 * dims[1] * dims[2];
  constexpr std::size_t numPtsPerCell = 8;
  this->NumCells = (adjustedDim0 - 1) * (dims[1] - 1) * (dims[2] - 1);

  int blockSize = 512;
  cudaStream_t pointStream, cellStream;
  cudaStreamCreate(&pointStream);
  cudaStreamCreate(&cellStream);
  switch (this->MemorySpace)
  {
    case SpaceTypes::Device:
    {
      // create the points -- slowest in the x and fastest in the z directions
      {
        std::size_t numElements = this->NumPoints * 3;
        int numberOfThreads = this->NumPoints;
        int numBlocks = (numberOfThreads + blockSize - 1) / blockSize;
        cudaMallocAsync(&(this->Points), numElements * sizeof(float), pointStream);
        ::GeneratePointCoordinates<<<numBlocks, blockSize, 0, pointStream>>>(
          startXPoint, dims[1], dims[2], this->Points, this->NumPoints);
      }
      // create the hex cells
      {
        std::size_t numElements = this->NumCells * numPtsPerCell;
        int numberOfThreads = this->NumCells;
        int numBlocks = (numberOfThreads + blockSize - 1) / blockSize;
        cudaMallocAsync(&(this->Cells), numElements * sizeof(int), cellStream);
        ::GenerateHexahedrons<<<numBlocks, blockSize, 0, cellStream>>>(
          dims[1], dims[2], this->Cells, this->NumCells);
      }
      break;
    }
    case SpaceTypes::Unified:
    default:
    {
      // create the points -- slowest in the x and fastest in the z directions
      {
        std::size_t numElements = this->NumPoints * 3;
        int numberOfThreads = this->NumPoints;
        int numBlocks = (numberOfThreads + blockSize - 1) / blockSize;
        cudaMallocManaged(&this->Points, numElements * sizeof(float));
        ::GeneratePointCoordinates<<<numBlocks, blockSize, 0, pointStream>>>(
          startXPoint, dims[1], dims[2], this->Points, this->NumPoints);
      }
      // create the hex cells
      {
        std::size_t numElements = this->NumCells * numPtsPerCell;
        int numberOfThreads = this->NumCells;
        int numBlocks = (numberOfThreads + blockSize - 1) / blockSize;
        cudaMallocManaged(&this->Cells, numElements * sizeof(int));
        ::GenerateHexahedrons<<<numBlocks, blockSize, 0, cellStream>>>(
          dims[1] - 1, dims[2] - 1, this->Cells, this->NumCells);
      }
      break;
    }
  }
  cudaStreamSynchronize(pointStream);
  cudaStreamDestroy(pointStream);
  cudaStreamSynchronize(cellStream);
  cudaStreamDestroy(cellStream);
}

size_t Grid::GetNumberOfPoints()
{
  return this->NumPoints;
}

size_t Grid::GetNumberOfCells()
{
  return this->NumCells;
}

float* Grid::GetPointsArray()
{
  return this->Points;
}

float* Grid::GetPoint(size_t pointId)
{
  if (pointId >= this->NumPoints)
  {
    return nullptr;
  }
  return &(this->Points[pointId * 3]);
}

int* Grid::GetCellPoints(size_t cellId)
{
  if (cellId >= this->NumCells)
  {
    return nullptr;
  }
  return &(this->Cells[cellId * 8]);
}

Attributes::Attributes()
{
  this->GridPtr = nullptr;
}

Attributes::~Attributes()
{
  if (this->Velocity != nullptr)
  {
    cudaFree(this->Velocity);
  }
  if (this->Pressure != nullptr)
  {
    cudaFree(this->Pressure);
  }
}

void Attributes::Initialize(Grid* grid)
{
  this->GridPtr = grid;
}

void Attributes::UpdateFields(double time)
{
  if (this->Velocity != nullptr)
  {
    cudaFree(this->Velocity);
    this->Velocity = nullptr;
  }
  if (this->Pressure != nullptr)
  {
    cudaFree(this->Pressure);
    this->Pressure = nullptr;
  }
  const size_t dims = this->GridPtr->GetNumberOfPoints();
  const size_t numCells = this->GridPtr->GetNumberOfCells();
  switch (this->GridPtr->GetMemorySpaceInUse())
  {
    case SpaceTypes::Device:
    {
      {
        size_t numElements = dims * 3;
        auto& devicePtr = this->Velocity;
        auto hostPtr = new float[numElements];
        auto hostCoordsPtr = new float[numElements];
        // get the point coordinates on the host.
        cudaMemcpy(hostCoordsPtr, this->GridPtr->GetPointsArray(), numElements * sizeof(float),
          cudaMemcpyDeviceToHost);
        // setup allocation for velocity on the device.
        cudaMalloc(&devicePtr, numElements * sizeof(float));
        // initialize data on host.
        for (size_t pt = 0; pt < dims; pt++)
        {
          float* coord = &hostCoordsPtr[pt * 3];
          hostPtr[pt] = coord[1] * time;
        }
        std::fill(hostPtr + dims, hostPtr + numElements, 0.);
        // upload velocity from host to device.
        cudaMemcpy(devicePtr, hostPtr, numElements * sizeof(float), cudaMemcpyHostToDevice);
        free(hostPtr);
        free(hostCoordsPtr);
      }
      {
        auto& devicePtr = this->Pressure;
        auto hostPtr = new float[numCells];
        // setup allocation for velocity on the device.
        cudaMalloc(&devicePtr, numCells * sizeof(float));
        std::fill(hostPtr, hostPtr + numCells, 1.0f);
        cudaMemcpy(devicePtr, hostPtr, numCells * sizeof(float), cudaMemcpyHostToDevice);
        free(hostPtr);
      }
      break;
    }
    case SpaceTypes::Unified:
    default:
    {
      {
        size_t numElements = dims * 3;
        cudaMallocManaged(&this->Velocity, numElements * sizeof(float));
        for (size_t pt = 0; pt < dims; pt++)
        {
          float* coord = this->GridPtr->GetPoint(pt);
          this->Velocity[pt] = coord[1] * time;
        }
        std::fill(this->Velocity + dims, this->Velocity + dims * 3, 0.);
      }
      {
        cudaMallocManaged(&this->Pressure, numCells * sizeof(float));
        std::fill(this->Pressure, this->Pressure + numCells, 1.f);
      }
      break;
    }
  }
}

float* Attributes::GetVelocityArray()
{
  return this->Velocity;
}

float* Attributes::GetPressureArray()
{
  return this->Pressure;
}
