/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChannelSubscription.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkChannelSubscription.h"

#include "vtkObjectFactory.h"
#include "vtkPacket.h"
#include "vtkServiceEndpoint.h"
#include "vtkSmartPointer.h"

class vtkChannelSubscription::vtkInternals
{
public:
  rxcpp::observable<vtkPacket> Observable;
  vtkSmartPointer<vtkServiceEndpoint> Endpoint;
  vtkEventual<bool> Status;
  std::string ChannelName;
};

vtkStandardNewMacro(vtkChannelSubscription);
//----------------------------------------------------------------------------
vtkChannelSubscription::vtkChannelSubscription()
  : Internals(new vtkChannelSubscription::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkChannelSubscription::~vtkChannelSubscription()
{
  auto& internals = (*this->Internals);
  if (internals.Endpoint)
  {
    internals.Endpoint->Unsubscribe(internals.ChannelName);
  }
}

//----------------------------------------------------------------------------
void vtkChannelSubscription::Initialize(vtkServiceEndpoint* endpoint, const std::string& channel,
  rxcpp::observable<vtkPacket>&& observable, const vtkEventual<bool>& status)
{
  auto& internals = (*this->Internals);
  internals.Endpoint = endpoint;
  internals.ChannelName = channel;
  internals.Observable = observable;
  internals.Status = status;
}

//----------------------------------------------------------------------------
const std::string& vtkChannelSubscription::GetChannelName() const
{
  const auto& internals = (*this->Internals);
  return internals.ChannelName;
}

//----------------------------------------------------------------------------
bool vtkChannelSubscription::Wait() const
{
  auto& internals = (*this->Internals);
  return internals.Status.Wait();
}

//----------------------------------------------------------------------------
rxcpp::observable<vtkPacket> vtkChannelSubscription::GetObservable() const
{
  auto& internals = (*this->Internals);
  return internals.Observable;
}

//----------------------------------------------------------------------------
vtkServiceEndpoint* vtkChannelSubscription::GetServiceEndpoint() const
{
  auto& internals = (*this->Internals);
  return internals.Endpoint;
}

//----------------------------------------------------------------------------
void vtkChannelSubscription::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
