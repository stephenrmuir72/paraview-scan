/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCommand.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMCommand
 * @brief extends vtkCommand to add events fired by ServerManager
 *
 * vtkSMCommand acts as a single place to define all events fired by
 * ServerManager.
 */

#ifndef vtkSMCommand_h
#define vtkSMCommand_h

#include "vtkCommand.h"
#include "vtkServerManagerCoreModule.h" // for exports

class VTKSERVERMANAGERCORE_EXPORT vtkSMCommand : public vtkCommand
{
public:
  vtkBaseTypeMacro(vtkSMCommand, vtkCommand);

  enum SMEventIds
  {
    StartEventMarker = vtkCommand::UserEvent,
    ConsumersChangedEvent,
  };

protected:
  vtkSMCommand();
  ~vtkSMCommand() override;

private:
  vtkSMCommand(const vtkSMCommand&) = delete;
  void operator=(const vtkSMCommand&) = delete;
};

#endif
