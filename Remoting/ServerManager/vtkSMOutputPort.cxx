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
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkRemoteObjectProvider.h"
#include "vtkSMSourceProxy.h"
#include "vtkTimerLog.h"

#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMOutputPort);

//----------------------------------------------------------------------------
vtkSMOutputPort::vtkSMOutputPort()
{
  this->DataInformation = vtkPVDataInformation::New();
  this->TemporalDataInformation = vtkPVTemporalDataInformation::New();
  this->PortIndex = 0;
  this->SourceProxy = nullptr;
  //  this->CompoundSourceProxy = nullptr;
  this->ObjectsCreated = 1;
}

//----------------------------------------------------------------------------
vtkSMOutputPort::~vtkSMOutputPort()
{
  this->SetSourceProxy(nullptr);
  this->DataInformation->Delete();
  this->TemporalDataInformation->Delete();
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::SetPortIndex(int index)
{
  this->PortIndex = index;
  this->DataInformation->SetPortNumber(this->PortIndex);
  this->TemporalDataInformation->SetPortNumber(this->PortIndex);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetDataInformation()
{
  return this->DataInformation;
}

//----------------------------------------------------------------------------
vtkPVTemporalDataInformation* vtkSMOutputPort::GetTemporalDataInformation()
{
  return this->TemporalDataInformation;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetSubsetDataInformation(
  const char* selector, const char* assemblyName)
{
#if 0
  auto dinfo = this->GetDataInformation();
  auto assembly = dinfo->GetDataAssembly(assemblyName);
  if (assembly == nullptr || selector == nullptr || selector[0] == '\0')
  {
    return nullptr;
  }

  const auto nodes = assembly->SelectNodes({ selector });
  if (nodes.empty())
  {
    return nullptr;
  }
  if (nodes.size() > 1)
  {
    vtkWarningMacro(
      "GetSubsetDataInformation selector matched multiple nodes. Only first one is used.");
  }

  const std::string key(assemblyName ? assemblyName : "");

  auto iter1 = this->SubsetDataInformations.find(key);
  if (iter1 != this->SubsetDataInformations.end())
  {
    auto iter2 = iter1->second.find(nodes.front());
    if (iter2 != iter1->second.end())
    {
      return iter2->second;
    }
  }

  this->SourceProxy->GetSession()->PrepareProgress();

  vtkNew<vtkPVDataInformation> subsetInfo;
  subsetInfo->Initialize();
  subsetInfo->SetPortNumber(this->PortIndex);
  subsetInfo->SetSubsetSelector(selector);
  subsetInfo->SetSubsetAssemblyName(assemblyName);
  this->SourceProxy->GatherInformation(subsetInfo);

  this->SubsetDataInformations[key][nodes.front()] = subsetInfo;
  this->SourceProxy->GetSession()->CleanupPendingProgress();
  return subsetInfo;
#else
  abort();
#endif
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetSubsetDataInformation(unsigned int compositeIndex)
{
#if 0
  auto dinfo = this->GetDataInformation();
  if (dinfo->DataSetTypeIsA(VTK_MULTIBLOCK_DATA_SET))
  {
    if (compositeIndex == 0)
    {
      // Bug #20997 return the full data information for composite index 0
      return dinfo;
    }
    auto hierarchy = dinfo->GetHierarchy();
    return this->GetSubsetDataInformation(
      vtkDataAssemblyUtilities::GetSelectorForCompositeId(compositeIndex, hierarchy).c_str(),
      vtkDataAssemblyUtilities::HierarchyName());
  }

  vtkWarningMacro("GetSelectorForCompositeId(compositeIndex) called for a non-multiblock dataset.");
  return nullptr;
#else
  abort();
#endif
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMOutputPort::GetRankDataInformation(int rank)
{
#if 0
  auto session = this->GetSession();
  const auto numRanks = session->GetNumberOfProcesses(this->GetSourceProxy()->GetLocation());
  if (rank < 0 || (numRanks == 1 && rank == 0))
  {
    // same as regular data information.
    return this->GetDataInformation();
  }

  if (rank >= numRanks)
  {
    // raise an error, since this may not be what the developer expected.
    vtkErrorMacro("Incorrect rank requested!");
    return this->GetDataInformation();
  }

  auto iter = this->RankDataInformations.find(rank);
  if (iter != this->RankDataInformations.end())
  {
    return iter->second;
  }

  this->SourceProxy->GetSession()->PrepareProgress();

  vtkNew<vtkPVDataInformation> rankInfo;
  rankInfo->Initialize();
  rankInfo->SetPortNumber(this->PortIndex);
  rankInfo->SetRank(rank);
  this->SourceProxy->GatherInformation(rankInfo);
  this->RankDataInformations[rank] = rankInfo;
  this->SourceProxy->GetSession()->CleanupPendingProgress();
  return rankInfo;
#else
  abort();
#endif
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PortIndex: " << this->PortIndex << endl;
  os << indent << "SourceProxy: " << this->SourceProxy << endl;
}

//----------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMOutputPort::GetSourceProxy()
{
#if 0
  return this->CompoundSourceProxy ? this->CompoundSourceProxy.GetPointer()
                                   : this->SourceProxy.GetPointer();
#else
  return this->SourceProxy;
#endif
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::SetSourceProxy(vtkSMSourceProxy* src)
{
  this->SourceProxy = src;
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::SetCompoundSourceProxy(vtkSMCompoundSourceProxy* src)
{
  //  this->CompoundSourceProxy = src;
}

//----------------------------------------------------------------------------
void vtkSMOutputPort::PipelineUpdated()
{
  // This is called when the VTK algorithm has updated. We simply flag the data
  // information obsolete.
  // DataInformation is already updated; see
  // vtkSMSessionProxyManager::ProcessPiggybackedMessages
  // this->DataInformation->SetIsObsolete(true);
  this->TemporalDataInformation->SetIsObsolete(true);
  this->SubsetDataInformations.clear();
  this->RankDataInformations.clear();
  this->InvokeEvent(vtkCommand::UpdateInformationEvent, this);
}
