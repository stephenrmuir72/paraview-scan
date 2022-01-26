/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVServerOptions
 * @brief
 *
 */

#ifndef vtkPVServerOptions_h
#define vtkPVServerOptions_h

#include "vtkPVApplicationOptions.h"

class VTKSERVERMANAGERCORE_EXPORT vtkPVServerOptions : public vtkPVApplicationOptions
{
public:
  static vtkPVServerOptions* New();
  vtkTypeMacro(vtkPVServerOptions, vtkPVApplicationOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the port number to listen for client connection on.
   */
  void SetPort(int port);
  int GetPort() const;
  ///@}

  void Populate(CLI::App& app) override;

protected:
  vtkPVServerOptions();
  ~vtkPVServerOptions();

private:
  vtkPVServerOptions(const vtkPVServerOptions&) = delete;
  void operator=(const vtkPVServerOptions&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
