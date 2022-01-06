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

#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"

class vtkSMOutputPort::vtkInternals
{
public:
  vtkSMProxy* Producer{ nullptr };
  int Index{ 0 };

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
void vtkSMOutputPort::SetProducer(vtkSMProxy* producer)
{
  auto& internals = (*this->Internals);
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
  internals.Index = index;
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
