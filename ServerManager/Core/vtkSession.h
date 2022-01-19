/*=========================================================================

  Program:   ParaView
  Module:    vtkSession.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSession
 * @brief abstract base class for all sessions.
 *
 * vtkSession acts as an abstract base class for all sessions in ParaView. A
 * session is a collection of services and/or endpoints that perform specific
 * duties in the application.
 */

#ifndef vtkSession_h
#define vtkSession_h

#include "vtkObject.h"
#include "vtkServerManagerCoreModule.h" //needed for exports

#include <memory> // for std::unique_ptr

class vtkProxyDefinitionManager;

class VTKSERVERMANAGERCORE_EXPORT vtkSession : public vtkObject
{
public:
  vtkTypeMacro(vtkSession, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the session's proxy definition manager.
   */
  vtkProxyDefinitionManager* GetProxyDefinitionManager() const;

protected:
  vtkSession();
  ~vtkSession() override;

private:
  vtkSession(const vtkSession&) = delete;
  void operator=(const vtkSession&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
