/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerApplication.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerApplication.h"

#include "vtkObjectFactory.h"
#include "vtkPVServerOptions.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkServerSession.h"
#include "vtkServicesEngine.h"

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

class vtkPVServerApplication::vtkInternals
{
public:
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  vtkSmartPointer<vtkServerSession> Session;
};

vtkStandardNewMacro(vtkPVServerApplication);
//----------------------------------------------------------------------------
vtkPVServerApplication::vtkPVServerApplication()
  : vtkPVCoreApplication(vtkPVCoreApplication::SERVER)
  , Internals(new vtkPVServerApplication::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVServerApplication::~vtkPVServerApplication() = default;

//----------------------------------------------------------------------------
vtkPVServerApplication* vtkPVServerApplication::GetInstance()
{
  return vtkPVServerApplication::SafeDownCast(vtkPVCoreApplication::GetInstance());
}

//----------------------------------------------------------------------------
vtkPVServerOptions* vtkPVServerApplication::GetOptions() const
{
  return vtkPVServerOptions::SafeDownCast(this->Superclass::GetOptions());
}

//----------------------------------------------------------------------------
bool vtkPVServerApplication::InitializeInternal()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.Session = vtk::TakeSmartPointer(vtkServerSession::New());
  internals.Session->StartServices();
  return true;
}

//----------------------------------------------------------------------------
void vtkPVServerApplication::FinalizeInternal()
{
  auto& internals = (*this->Internals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);
  internals.Session = nullptr;
}

//----------------------------------------------------------------------------
std::string vtkPVServerApplication::GetEngineURL() const
{
  auto* options = this->GetOptions();
  // FIXME: this doesn't work as expected.
  return fmt::format("sockets://{}:{}", options->GetLocalHostname(), options->GetPort());
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPVApplicationOptions> vtkPVServerApplication::CreateOptions() const
{
  return vtk::TakeSmartPointer(vtkPVServerOptions::New());
}

//----------------------------------------------------------------------------
void vtkPVServerApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
