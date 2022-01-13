/*=========================================================================

  Program:   ParaView
  Module:    vtkClientSession.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkClientSession
 * @brief session for a ParaView client.
 *
 */

#ifndef vtkClientSession_h
#define vtkClientSession_h

#include "vtkEventual.h" // for vtkEventual
#include "vtkSession.h"

#include <memory> // for std::unique_ptr

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkSMProxyManager;
class vtkPacket;

class VTKREMOTINGCORE_EXPORT vtkClientSession : public vtkSession
{
public:
  static vtkClientSession* New();
  vtkTypeMacro(vtkClientSession, vtkSession);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ServiceTypes
  {
    DATA_SERVER = 0x01,
    RENDER_SERVER = 0x02,
    CLIENT = 0x04
  };

  /**
   * Connect to a server session.
   */
  rxcpp::observable<bool> Connect(const std::string& url = std::string{});

  /**
   * Provides access to the session's proxy-manager.
   */
  vtkSMProxyManager* GetProxyManager() const;

  /**
   * Send a message to services identified by the destination
   */
  void SendMessage(vtkTypeUInt32 destination, const vtkPacket& packet) const;

  /**
   * Sends a request and returns a response. If destination identifies multiple
   * services, the "root" location is used.
   */
  vtkEventual<vtkPacket> SendRequest(vtkTypeUInt32 destination, const vtkPacket& packet) const;

  static vtkTypeUInt32 GetRootDestination(vtkTypeUInt32 destination);

  /**
   * A convenience method to get the scheduler to use to schedule actions
   * on the applications main thread.
   */
  rxcpp::observe_on_one_worker GetCoordination() const;

protected:
  vtkClientSession();
  ~vtkClientSession() override;

private:
  vtkClientSession(const vtkClientSession&) = delete;
  void operator=(const vtkClientSession&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
