/*=========================================================================

  Program:   ParaView
  Module:    vtkDistributedEnvironment.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDistributedEnvironment
 * @brief class to help with MPI initialization
 *
 * For ParaView apps supporting MPI, this class helps with the MPI
 * initialization and cleanup. Typically, this is the first object one must
 * create before processing command line arguments. MPI implementations may
 * process and modify command line arguments and hence MPI must be given a
 * change to process them before application specific command line argument
 * processing can be done.
 */

#ifndef vtkDistributedEnvironment_h
#define vtkDistributedEnvironment_h

#include "vtkRemotingServerManagerModule.h" // for exports
#include <memory>                           // for unique_ptr

class vtkMultiProcessController;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkDistributedEnvironment
{
public:
  vtkDistributedEnvironment(int* argc, char*** argv);
  ~vtkDistributedEnvironment();

  /**
   * Provides access to the multiprocess controller for the "world".
   */
  vtkMultiProcessController* GetController() const;

private:
  vtkDistributedEnvironment(const vtkDistributedEnvironment&) = delete;
  void operator=(const vtkDistributedEnvironment&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
