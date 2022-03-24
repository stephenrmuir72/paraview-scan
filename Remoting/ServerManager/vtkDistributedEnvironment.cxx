/*=========================================================================

  Program:   ParaView
  Module:    vtkDistributedEnvironment.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistributedEnvironment.h"

#include "vtkDummyController.h"
#include "vtkLogger.h"
#include "vtkSmartPointer.h"

#if VTK_MODULE_ENABLE_VTK_mpi && VTK_MODULE_ENABLE_VTK_ParallelMPI
#define VTKPTMPIHELPER_USE_MPI 1
#else
#define VTKPTMPIHELPER_USE_MPI 0
#endif

#if VTKPTMPIHELPER_USE_MPI
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtk_mpi.h"
#endif

class vtkDistributedEnvironment::vtkInternals
{
public:
  vtkSmartPointer<vtkMultiProcessController> Controller;
  bool MPIInitialized{ false };
  bool MPIExternallyInitialized{ false };
};

//----------------------------------------------------------------------------
vtkDistributedEnvironment::vtkDistributedEnvironment(int* argc, char*** argv)
  : Internals(new vtkDistributedEnvironment::vtkInternals())
{
  auto& internals = (*this->Internals);
#if VTKPTMPIHELPER_USE_MPI
  int flag;
  MPI_Initialized(&flag);
  internals.MPIInitialized = (flag != 0);
  internals.MPIExternallyInitialized = internals.MPIInitialized;
  if (!internals.MPIInitialized)
  {
    int provided_threading;
    vtkLogF(TRACE, "Initializing MPI");
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided_threading);
    internals.MPIInitialized = true;
  }

  // assert(internals.MPIInitialized);
  int provided_threading;
  MPI_Query_thread(&provided_threading);
  if (provided_threading != MPI_THREAD_MULTIPLE)
  {
    throw std::runtime_error("MPI_THREAD_MULTIPLE not supported!");
  }

  auto controller = vtk::TakeSmartPointer(vtkMPIController::New());
  controller->Initialize(argc, argv, /*initializedExternally*/ 1);
  // ensure we use broadcast/collectives for RMI rather than p2p messaging.
  controller->BroadcastTriggerRMIOn();
  internals.Controller = std::move(controller);
#else
  internals.Controller = vtk::TakeSmartPointer(vtkDummyController::New());
  internals.Controller->Initialize(argc, argv);
#endif
}

//----------------------------------------------------------------------------
vtkDistributedEnvironment::~vtkDistributedEnvironment()
{
#if VTKPTMPIHELPER_USE_MPI
  auto& internals = (*this->Internals);
  internals.Controller->Finalize(/*finalizedExternally*/ 1);
  internals.Controller = nullptr;

  if (internals.MPIInitialized && !internals.MPIExternallyInitialized)
  {
    vtkLogF(TRACE, "finalizing MPI");
    MPI_Finalize();
  }
#endif
}

//----------------------------------------------------------------------------
int vtkDistributedEnvironment::GetRank() const
{
  return this->GetController()->GetLocalProcessId();
}

//----------------------------------------------------------------------------
int vtkDistributedEnvironment::GetNumberOfRanks() const
{
  return this->GetController()->GetNumberOfProcesses();
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkDistributedEnvironment::GetController() const
{
  auto& internals = (*this->Internals);
  return internals.Controller;
}
