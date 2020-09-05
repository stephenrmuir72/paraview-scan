#include "FEDataStructures.h"
#include <mpi.h>

#ifdef USE_CATALYST
#include "CatalystAdaptor.h"
#endif

// Example of a C adaptor for a simulation code
// where the simulation code has a fixed topology
// grid. We treat the grid as an unstructured
// grid even though in the example provided it
// would be best described as a vtkImageData.
// Also, the points are stored in an inconsistent
// manner with respect to the velocity vector.
// This is purposefully done to demonstrate
// the different approaches for getting data
// into Catalyst. In this example we don't
// use any of the Fortran/C API provided in
// Catalyst. That is in CFullExample2.
// Note that through configuration
// that the driver can be run without linking
// to Catalyst.

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  int rank, nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  Grid grid = (Grid){.NumberOfPoints = 0, .Points = 0, .NumberOfCells = 0, .Cells = 0 };
  unsigned int numPoints[3] = { 70, 60, 44 };
  double spacing[3] = { 1, 1.1, 1.3 };
  InitializeGrid(&grid, numPoints, spacing);
  Attributes attributes;
  InitializeAttributes(&attributes, &grid);

#ifdef USE_CATALYST
  do_catalyst_initialization(MPI_COMM_WORLD, argc, argv);
#endif
  unsigned int numberOfTimeSteps = 5;
  unsigned int timeStep;
  for (timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    UpdateFields(&attributes, time);

    const int color = (rank == 0 || ((timeStep * rank) % 2 == 0)) ? 0 : MPI_UNDEFINED;

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm subcomm;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &subcomm);

#ifdef USE_CATALYST
    if (subcomm != MPI_COMM_NULL)
    {
      int sub_rank, sub_nprocs;
      MPI_Comm_rank(subcomm, &sub_rank);
      MPI_Comm_size(subcomm, &sub_nprocs);
      if (sub_rank == 0)
      {
        printf("timeStep=%d, subgroup nrank=%d\n", timeStep, sub_nprocs);
      }
      do_catalyst_execute(subcomm, timeStep, time, &grid, &attributes);
      MPI_Comm_free(&subcomm);
    }
#endif
    MPI_Barrier(MPI_COMM_WORLD);
  }

#ifdef USE_CATALYST
  do_catalyst_finalization(MPI_COMM_WORLD);
#endif

  MPI_Finalize();
  return 0;
}
