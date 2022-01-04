/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChannelSubscription.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkChannelSubscription
 * @brief
 *
 */

#ifndef vtkChannelSubscription_h
#define vtkChannelSubscription_h

#include "vtkEventual.h" // for vtkEventual
#include "vtkObject.h"
#include "vtkServicesCoreModule.h" // for exports

#include <memory> // for std::unique_ptr

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkPacket;
class vtkService;
class vtkServiceEndpoint;

class VTKSERVICESCORE_EXPORT vtkChannelSubscription : public vtkObject
{
public:
  vtkTypeMacro(vtkChannelSubscription, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkChannelSubscription(const vtkChannelSubscription&) = delete;
  void operator=(const vtkChannelSubscription&) = delete;

  /**
   * Returns the name of this channel.
   */
  const std::string& GetChannelName() const;

  /**
   * Wait until the subscription is established. Returns the status of the
   * subscription.
   */
  bool Wait() const;

  /**
   * Get the observable assosiated with the channel
   */
  rxcpp::observable<vtkPacket> GetObservable() const;

  /**
   * Returns a pointer to the point this subscription is made on.
   */
  vtkServiceEndpoint* GetServiceEndpoint() const;

protected:
  vtkChannelSubscription();
  ~vtkChannelSubscription() override;

  friend class vtkServiceEndpoint;
  friend class vtkService;
  static vtkChannelSubscription* New();

  void Initialize(vtkServiceEndpoint* endpoint, const std::string& channel,
    rxcpp::observable<vtkPacket>&& observable, const vtkEventual<bool>& status);

private:
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
