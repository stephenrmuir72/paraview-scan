/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerOptions.h"

#include "vtkObjectFactory.h"

#include <vtk_cli11.h>

class vtkPVServerOptions::vtkInternals
{
public:
  int Port{ 33333 };
  bool ReverseConnection{ false };
  std::string ClientHostName;
  int Timeout{ 0 };
  std::string TimeoutCommand;
};

#define CHECK_READ_ONLY()                                                                          \
  vtkLogIfF(ERROR, this->GetReadOnly(), "Options is read-only!");                                  \
  if (this->GetReadOnly())                                                                         \
  {                                                                                                \
    return;                                                                                        \
  }

vtkStandardNewMacro(vtkPVServerOptions);
//----------------------------------------------------------------------------
vtkPVServerOptions::vtkPVServerOptions()
  : Internals(new vtkPVServerOptions::vtkInternals())
{
  auto& internals = (*this->Internals);
  internals.ClientHostName = this->GetLocalHostname();
}

//----------------------------------------------------------------------------
vtkPVServerOptions::~vtkPVServerOptions() = default;

//----------------------------------------------------------------------------
void vtkPVServerOptions::SetPort(int port)
{
  CHECK_READ_ONLY();

  auto& internals = (*this->Internals);
  internals.Port = port;
}

//----------------------------------------------------------------------------
int vtkPVServerOptions::GetPort() const
{
  const auto& internals = (*this->Internals);
  return internals.Port;
}

//----------------------------------------------------------------------------
int vtkPVServerOptions::GetTimeout() const
{
  const auto& internals = (*this->Internals);
  return internals.Timeout;
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::SetTimeout(int minutes)
{
  CHECK_READ_ONLY();

  auto& internals = (*this->Internals);
  internals.Timeout = minutes;
}

//----------------------------------------------------------------------------
const std::string& vtkPVServerOptions::GetTimeoutCommand() const
{
  const auto& internals = (*this->Internals);
  return internals.TimeoutCommand;
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::SetTimeoutCommand(const std::string& command)
{
  CHECK_READ_ONLY();

  auto& internals = (*this->Internals);
  internals.TimeoutCommand = command;
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::Populate(CLI::App& app)
{
  CHECK_READ_ONLY();

  this->Superclass::Populate(app);
  auto& internals = (*this->Internals);

  auto* groupServer = app.add_option_group("Server", "Server options");
  groupServer
    ->add_option("--client-host", internals.ClientHostName,
      "Hostname to use to connect to the ParaView client when using reverse-connection mode. "
      "Defaults to 'localhost'.")
    ->default_val(internals.ClientHostName);

  groupServer
    ->add_flag("-r,--rc,--reverse-connection", internals.ReverseConnection,
      "Use reverse connection mode, i.e. instead of client connecting to the server(s), "
      "the server(s) will connect back to the client.")
    ->default_val(internals.ReverseConnection);

  groupServer
    ->add_option("-p,--sp,--server-port,--port", internals.Port,
      "Port number to use to listen for connections from the client. In reverse-connection mode, "
      "this is the port number on which the client is listening for connections.")
    ->default_val(internals.Port);

  groupServer
    ->add_option("--timeout", internals.Timeout,
      "Time interval (in minutes) since a connection is established that the server-connection "
      "may timeout. "
      "The client typically shows a warning message before the server times out.")
    ->default_val(0);

  groupServer->add_option("--timeout-command", internals.TimeoutCommand,
    "Timeout command allowing server to regularly check remaining time available.");
}

//----------------------------------------------------------------------------
void vtkPVServerOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
