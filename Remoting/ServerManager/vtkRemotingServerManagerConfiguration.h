/*=========================================================================

  Program:   ParaView
  Module:    vtkRemotingServerManagerConfiguration.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkRemotingServerManagerConfiguration
 * @brief runtime configuration options for vtkRemotingServerManager module.
 *
 * vtkRemotingServerManagerConfiguration is a singleton that maintains runtime
 * configuration for the process that affects the Server-Manager.
 */

#ifndef vtkRemotingServerManagerConfiguration_h
#define vtkRemotingServerManagerConfiguration_h

#include "vtkProcessModule.h"               // for vtkProcessModule::ProcessTypes
#include "vtkRemotingServerManagerModule.h" // for exports
#include "vtkSMObject.h"

class vtkCLIOptions;
class VTKREMOTINGSERVERMANAGER_EXPORT vtkRemotingServerManagerConfiguration : public vtkSMObject
{
public:
  vtkTypeMacro(vtkRemotingServerManagerConfiguration, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the singleton.
   */
  static vtkRemotingServerManagerConfiguration* GetInstance();

  /**
   * Get whether to use multi-client mode. In multi-client mode,
   * a server can accept connections from multiple clients at the same time.
   * This is also referred to as collaboration mode. This mode is largely
   * experimental and may not be supported in the future. Hence it is
   * recommended that newer applications avoid reliance on this capability.
   */
  vtkGetMacro(MultiClientMode, bool);

  /**
   * Populates vtkCLIOptions with available command line options.
   * `processType` indicates which type of ParaView process the options are
   * being setup for. That may affect available options.
   */
  bool PopulateOptions(vtkCLIOptions* app, vtkProcessModule::ProcessTypes processType);

protected:
  vtkRemotingServerManagerConfiguration();
  ~vtkRemotingServerManagerConfiguration();

private:
  vtkRemotingServerManagerConfiguration(const vtkRemotingServerManagerConfiguration&) = delete;
  void operator=(const vtkRemotingServerManagerConfiguration&) = delete;
  static vtkRemotingServerManagerConfiguration* New();
  bool MultiClientMode;
};

#endif
