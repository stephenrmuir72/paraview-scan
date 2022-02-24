/*=========================================================================

   Program: ParaView
   Module:  vtkPVLogInformation.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "vtkPVLogInformation.h"

#include "vtkLogRecorder.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVLogInformation);
//----------------------------------------------------------------------------
void vtkPVLogInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Rank: " << this->Rank << endl;
  os << indent << "Logs: " << this->Logs << endl;
  os << indent << "Starting Logs: " << this->StartingLogs << endl;
  os << indent << "Verbosity" << this->Verbosity << endl;
}

//----------------------------------------------------------------------------
bool vtkPVLogInformation::GatherInformation(vtkObject* obj)
{
  if (!obj)
  {
    vtkErrorMacro("Cannot get class name from NULL object.");
    return false;
  }

  vtkLogRecorder* logRecorder = vtkLogRecorder::SafeDownCast(obj);
  if (logRecorder == nullptr)
  {
    vtkErrorMacro("Cannot down cast to vtkLogRecorder.");
    return false;
  }

  if (this->Rank == logRecorder->GetMyRank())
  {
    this->Logs = logRecorder->GetLogs();
    this->StartingLogs = logRecorder->GetStartingLog();
  }
  this->Verbosity = vtkLogger::GetCurrentVerbosityCutoff();
  return true;
}

//----------------------------------------------------------------------------
void vtkPVLogInformation::AddInformation(vtkPVInformation* info)
{
  if (auto* pvinfo = vtkPVLogInformation::SafeDownCast(info))
  {
    this->Logs += pvinfo->Logs;
    this->StartingLogs += pvinfo->StartingLogs;
    if (this->Verbosity == 20)
    {
      this->Verbosity = pvinfo->Verbosity;
    }
  }
}

//----------------------------------------------------------------------------
vtkNJson vtkPVLogInformation::SaveState() const
{
  auto json = this->Superclass::SaveState();
  VTK_NJSON_SAVE_MEMBER(json, Rank);
  return json;
}

//----------------------------------------------------------------------------
bool vtkPVLogInformation::LoadState(const vtkNJson& json)
{
  if (!this->Superclass::LoadState(json))
  {
    return false;
  }
  VTK_NJSON_LOAD_MEMBER(json, Rank);
  return true;
}

//----------------------------------------------------------------------------
vtkNJson vtkPVLogInformation::SaveInformation() const
{
  vtkNJson json;
  VTK_NJSON_SAVE_MEMBER(json, Logs);
  VTK_NJSON_SAVE_MEMBER(json, StartingLogs);
  VTK_NJSON_SAVE_MEMBER(json, Verbosity);
  return json;
}

//----------------------------------------------------------------------------
bool vtkPVLogInformation::LoadInformation(const vtkNJson& json)
{
  VTK_NJSON_LOAD_MEMBER(json, Logs);
  VTK_NJSON_LOAD_MEMBER(json, StartingLogs);
  VTK_NJSON_LOAD_MEMBER(json, Verbosity);
  return true;
}
