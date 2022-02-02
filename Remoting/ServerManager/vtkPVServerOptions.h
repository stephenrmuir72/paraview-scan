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

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVServerOptions : public vtkPVApplicationOptions
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

  ///@{
  /**
   * Get/Set timeout. This denotes the time (in minutes)
   * since the time that the connection was established with the server that the
   * server may timeout. timeout <= 0 means no timeout.
   */
  int GetTimeout() const;
  void SetTimeout(int minutes);
  ///@}

  ///@{
  /**
   * Get/Set the timeout command, called regularly on server side and giving
   * remaining time available for server access.
   */
  const std::string& GetTimeoutCommand() const;
  void SetTimeoutCommand(const std::string& command);
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
