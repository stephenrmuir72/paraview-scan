/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumServiceEndpoint.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkThalliumServiceEndpoint
 * @brief
 *
 */

#ifndef vtkThalliumServiceEndpoint_h
#define vtkThalliumServiceEndpoint_h

#include "vtkServiceEndpoint.h"
#include "vtkServicesThalliumModule.h" // for exports

class VTKSERVICESTHALLIUM_EXPORT vtkThalliumServiceEndpoint : public vtkServiceEndpoint
{
public:
  static vtkThalliumServiceEndpoint* New();
  vtkTypeMacro(vtkThalliumServiceEndpoint, vtkServiceEndpoint);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkThalliumServiceEndpoint(const vtkThalliumServiceEndpoint&) = delete;
  vtkThalliumServiceEndpoint& operator=(const vtkThalliumServiceEndpoint&) = delete;

protected:
  vtkThalliumServiceEndpoint();
  ~vtkThalliumServiceEndpoint() override;

  void InitializeInternal() override;
  vtkEventual<bool> ConnectInternal(
    const std::string& serviceName, const std::string& url) override;
  void ShutdownInternal() override;
  void SendMessageInternal(const vtkPacket& packet) const override;
  vtkEventual<vtkPacket> SendRequestInternal(const vtkPacket& packet) const override;

  vtkEventual<bool> SubscribeInternal(const std::string& channel) override;
  void UnsubscribeInternal(const std::string& channel) override;

private:
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  class vtkProvider;
  friend class vtkThalliumChannelSubscription;
};

#endif
