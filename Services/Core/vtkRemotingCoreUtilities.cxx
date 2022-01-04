/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemotingCoreUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRemotingCoreUtilities.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkRemotingCoreUtilities);
//----------------------------------------------------------------------------
vtkRemotingCoreUtilities::vtkRemotingCoreUtilities() = default;

//----------------------------------------------------------------------------
vtkRemotingCoreUtilities::~vtkRemotingCoreUtilities() = default;

//----------------------------------------------------------------------------
void vtkRemotingCoreUtilities::EnsureThread(const std::thread::id& owner)
{
  if (std::this_thread::get_id() != owner)
  {
    vtkLogF(ERROR, "called non-thread safe API on different threads!");
    abort();
  }
}

//----------------------------------------------------------------------------
void vtkRemotingCoreUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
