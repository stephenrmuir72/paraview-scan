/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVParameterProducer.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVParameterProducer.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkLiveInsituLink.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

vtkStandardNewMacro(vtkPVParameterProducer);
vtkCxxSetObjectMacro(vtkPVParameterProducer, Link, vtkLiveInsituLink);

vtkPVParameterProducer::vtkPVParameterProducer()
  : Link(nullptr)
{
}

vtkPVParameterProducer::~vtkPVParameterProducer()
{
  this->SetLink(nullptr);
}

void vtkPVParameterProducer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Link: " << this->Link << "\n";
}

void vtkPVParameterProducer::SetParameter(
  const char* parameter, vtkIdType tupleId, size_t numComps, double* tupleValues)
{
  if (!parameter)
  {
    return;
  }
  if (tupleId < 0)
  {
    return;
  }
  if (this->Link && this->Link->GetProcessType() == vtkLiveInsituLink::LIVE)
  {
    // On the ParaView side, nothing needs to be done; the
    // proxy object holds the values of interest and they are
    // transmitted back to the simulation when the apply button
    // is clicked via the vtkLiveInsituLink.
    //
    // std::cout << "Parameter " << parameter << " value (" << tupleValues[0] << " ...) updated\n";
    return;
  }
  bool didSet = false;
  auto dobj = this->GetOutputDataObject(0);
  auto fields = dobj ? dobj->GetFieldData() : nullptr;
  vtkDataArray* arr = fields ? fields->GetArray(parameter) : nullptr;
  // On the ParaView live (client) side, the field data will not be present and
  // thus no changes are made below. On the Catalyst (simulation) side, the
  // field data will be present and managed by the steering adaptor:
  if (arr && arr->GetNumberOfComponents() == static_cast<int>(numComps) &&
    tupleId < arr->GetNumberOfTuples())
  {
    // We need to be careful about modifying parameters only when there
    // is an actual change in value so we don't needlessly trigger work
    // for the simulation that has no effect.
    std::vector<double> oldValue(numComps);
    arr->GetTuple(tupleId, &oldValue[0]);
    bool changed = false;
    for (int ii = 0; ii < numComps; ++ii)
    {
      if (oldValue[ii] != tupleValues[ii])
      {
        changed = true;
        break;
      }
    }
    if (changed)
    {
      arr->SetTuple(tupleId, tupleValues);
      arr->Modified();
      didSet = true;
    }
  }
  if (!didSet)
  {
    // vtkErrorMacro("Could not set property " << parameter);
  }
}

void vtkPVParameterProducer::SetParameter(
  const char* parameter, vtkIdType tupleId, const char* tupleValue)
{
  if (!parameter)
  {
    return;
  }
  if (tupleId < 0)
  {
    return;
  }
  if (this->Link && this->Link->GetProcessType() == vtkLiveInsituLink::LIVE)
  {
    // On the ParaView side, nothing needs to be done; the
    // proxy object holds the values of interest and they are
    // transmitted back to the simulation when the apply button
    // is clicked via the vtkLiveInsituLink.
    //
    // std::cout << "Parameter " << parameter << " value (" << tupleValues[0] << " ...) updated\n";
    return;
  }
  bool didSet = false;
  auto dobj = this->GetOutputDataObject(0);
  auto fields = dobj ? dobj->GetFieldData() : nullptr;
  auto arr = dynamic_cast<vtkStringArray*>(fields ? fields->GetAbstractArray(parameter) : nullptr);
  // On the ParaView live (client) side, the field data will not be present and
  // thus no changes are made below. On the Catalyst (simulation) side, the
  // field data will be present and managed by the steering adaptor:
  if (arr && tupleId < arr->GetNumberOfValues())
  {
    // We need to be careful about modifying parameters only when there
    // is an actual change in value so we don't needlessly trigger work
    // for the simulation that has no effect.
    std::string oldValue = arr->GetValue(tupleId);
    bool changed = oldValue != tupleValue;
    if (changed)
    {
      arr->SetValue(tupleId, tupleValue);
      arr->Modified();
      didSet = true;
    }
  }
  if (!didSet)
  {
    // vtkErrorMacro("Could not set property " << parameter);
  }
}

vtkAbstractArray* vtkPVParameterProducer::GetParameter(const char* parameter)
{
  if (!parameter)
  {
    return nullptr;
  }
  if (this->Link && this->Link->GetProcessType() == vtkLiveInsituLink::LIVE)
  {
    // On the ParaView side, nothing needs to be done; the
    // proxy object holds the values of interest and they are
    // transmitted back to the simulation when the apply button
    // is clicked via the vtkLiveInsituLink.
    //
    // std::cout << "Parameter " << parameter << " value (" << tupleValues[0] << " ...) updated\n";
    std::cout << "Have LIVE side of link\n";
    return nullptr;
  }
  bool didSet = false;
  auto dobj = this->GetOutputDataObject(0);
  auto fields = dobj ? dobj->GetFieldData() : nullptr;
  vtkAbstractArray* arr = fields ? fields->GetAbstractArray(parameter) : nullptr;
  // On the ParaView live (client) side, the field data will not be present and
  // thus no changes are made below. On the Catalyst (simulation) side, the
  // field data will be present and managed by the steering adaptor:
  if (arr && arr->GetNumberOfComponents() > 0 && arr->GetNumberOfTuples() > 0)
  {
    return arr;
  }
  return nullptr;
}
