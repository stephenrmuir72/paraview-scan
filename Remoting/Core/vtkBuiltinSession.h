/*=========================================================================

  Program:   ParaView
  Module:    vtkBuiltinSession.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkBuiltinSession
 * @brief
 *
 */

#ifndef vtkBuiltinSession_h
#define vtkBuiltinSession_h

#include "vtkClientSession.h"

class VTKREMOTINGCORE_EXPORT vtkBuiltinSession : public vtkClientSession
{
public:
  static vtkBuiltinSession* New();
  vtkTypeMacro(vtkBuiltinSession, vtkClientSession);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkBuiltinSession();
  ~vtkBuiltinSession() override;

private:
  vtkBuiltinSession(const vtkBuiltinSession&) = delete;
  void operator=(const vtkBuiltinSession&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
