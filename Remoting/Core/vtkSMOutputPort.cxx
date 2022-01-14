/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOutputPort.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMOutputPort.h"

#include "vtkClientSession.h"
#include "vtkCommand.h"
#include "vtkGarbageCollector.h"
#include "vtkInformationProvider.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkSMProxy.h"

class vtkSMOutputPort::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkSMProxy* Producer{ nullptr };
  int Index{ 0 };
  vtkNew<vtkPVDataInformation> DataInformation;

  ~vtkInternals()
  {
    if (this->Producer)
    {
      this->Producer->UnRegister(nullptr);
    }
  }
};

vtkStandardNewMacro(vtkSMOutputPort);
//----------------------------------------------------------------------------
vtkSMOutputPort::vtkSMOutputPort()
  : Internals(new vtkSMOutputPort::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMOutputPort::~vtkSMOutputPort() = default;

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMOutputPort::GetProducer() const
{
  const auto& internals = (*this->Internals);
  return internals.Producer;
}

//----------------------------------------------------------------------------
int vtkSMOutputPort::GetIndex() const
{
  const auto& internals = (*this->Internals);
  return internals.Index;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetDataInformation() const
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  return internals.DataInformation;
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::SetProducer(vtkSMProxy* producer)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  if (internals.Producer == producer)
  {
    return;
  }
  if (internals.Producer)
  {
    internals.Producer->UnRegister(this);
  }
  internals.Producer = producer;
  if (internals.Producer)
  {
    internals.Producer->Register(this);
  }
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::SetIndex(int index)
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.Index = index;
  internals.DataInformation->SetPortNumber(index);
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::PipelineUpdated()
{
  const auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  // This is called when the VTK algorithm has updated. We simply flag the data
  // information obsolete.
  internals.DataInformation->SetIsObsolete(true);

  auto* producer = this->GetProducer();
  auto* session = producer->GetSession();

  vtkSmartPointer<vtkPVDataInformation> dinfo = internals.DataInformation.GetPointer();

  auto packet =
    vtkInformationProvider::GatherInformation(internals.DataInformation, producer->GetGlobalID());
  auto coordination = session->GetCoordination();
  auto eventual = session->SendRequest(
    /** FIXME**/ producer->GetLocation(), std::move(packet));
  eventual.GetObservable()
    .observe_on(coordination)
    .subscribe([dinfo, this](const vtkPacket& packet) {
      vtkLogF(INFO, "update data information");
      if (vtkInformationProvider::ParseResponse(packet, dinfo))
      {
        // FIXME: need better named events
        this->InvokeEvent(vtkCommand::UpdateInformationEvent, this);
      }
    });
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  auto& internals = (*this->Internals);
  vtkGarbageCollectorReport(collector, internals.Producer, "Producer");
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
