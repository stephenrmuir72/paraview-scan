/*=========================================================================

  Program:   ParaView
  Module:    vtkServerSession.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkServerSession
 * @brief session for a ParaView server
 *
 * vtkServerSession represents a ParaView server session. This session starts up
 * two services named "ds" and "rs". "ds" represents a service that takes on the
 * responsibilities for doing all the data processing while "rs" represents the
 * rendering service that does rendering.
 */

#ifndef vtkServerSession_h
#define vtkServerSession_h

#include "vtkSession.h"
#include <memory> // for std::unique_ptr

class VTKREMOTINGCORE_EXPORT vtkServerSession : public vtkSession
{
public:
  static vtkServerSession* New();
  vtkTypeMacro(vtkServerSession, vtkSession);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkServerSession();
  ~vtkServerSession() override;

private:
  vtkServerSession(const vtkServerSession&) = delete;
  void operator=(const vtkServerSession&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
