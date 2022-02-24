/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTimerInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTimerInformation.h"

#include "vtkByteSwap.h"
#include "vtkDataObject.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"

#include <sstream>
#include <string>

class vtkPVTimerInformation::vtkInternals
{
public:
  double LogThreshold{ 0 };
  std::vector<std::string> Logs;
};

vtkStandardNewMacro(vtkPVTimerInformation);
//----------------------------------------------------------------------------
vtkPVTimerInformation::vtkPVTimerInformation()
  : Internals(new vtkPVTimerInformation::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVTimerInformation::~vtkPVTimerInformation() = default;

//----------------------------------------------------------------------------
void vtkPVTimerInformation::SetLogThreshold(double threshold)
{
  auto& internals = (*this->Internals);
  internals.LogThreshold = threshold;
}

//----------------------------------------------------------------------------
double vtkPVTimerInformation::GetLogThreshold() const
{
  const auto& internals = (*this->Internals);
  return internals.LogThreshold;
}

//----------------------------------------------------------------------------
vtkNJson vtkPVTimerInformation::SaveState() const
{
  const auto& internals = (*this->Internals);

  vtkNJson json;
  json["threshold"] = internals.LogThreshold;
  return json;
}

//----------------------------------------------------------------------------
bool vtkPVTimerInformation::LoadState(const vtkNJson& state)
{
  auto iter = state.find("threshold");
  if (iter != state.end())
  {
    auto& internals = (*this->Internals);
    internals.LogThreshold = iter.value().get<double>();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
// This ignores the object, and gets the log from the timer.
bool vtkPVTimerInformation::GatherInformation(vtkObject*)
{
  const int length = vtkTimerLog::GetNumberOfEvents() * 40;
  if (length > 0)
  {
    auto& internals = (*this->Internals);

    std::ostringstream fptr;
    vtkTimerLog::DumpLogWithIndents(&fptr, static_cast<float>(internals.LogThreshold));
    internals.Logs.emplace_back(fptr.str());
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::AddInformation(vtkPVInformation* info)
{
  auto* other = vtkPVTimerInformation::SafeDownCast(info);
  if (!other)
  {
    return;
  }

  auto& internals = (*this->Internals);
  const auto& ointernals = (*other->Internals);
  internals.Logs.reserve(internals.Logs.size() + ointernals.Logs.size());
  std::copy(ointernals.Logs.begin(), ointernals.Logs.end(), std::back_inserter(internals.Logs));
}

//----------------------------------------------------------------------------
bool vtkPVTimerInformation::LoadInformation(const vtkNJson& json)
{
  auto& internals = (*this->Internals);
  auto iter = json.find("logs");
  if (iter != json.end())
  {
    internals.Logs = iter.value().get<std::vector<std::string>>();
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
vtkNJson vtkPVTimerInformation::SaveInformation() const
{
  const auto& internals = (*this->Internals);
  vtkNJson json;
  json["logs"] = internals.Logs;
  return json;
}

//----------------------------------------------------------------------------
size_t vtkPVTimerInformation::GetNumberOfLogs() const
{
  const auto& internals = (*this->Internals);
  return internals.Logs.size();
}

//----------------------------------------------------------------------------
const std::string& vtkPVTimerInformation::GetLog(size_t index) const
{
  const auto& internals = (*this->Internals);
  if (index < internals.Logs.size())
  {
    return internals.Logs.at(index);
  }

  static std::string tmp;
  return tmp;
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  const auto& internals = (*this->Internals);
  os << indent << "LogThreshold: " << internals.LogThreshold << endl;
  os << indent << "NumberOfLogs: " << this->GetNumberOfLogs() << endl;

  size_t index = 0;
  for (const auto& log : internals.Logs)
  {
    os << indent << "Log " << index++ << ": \n";
    os << log << endl;
  }
}
