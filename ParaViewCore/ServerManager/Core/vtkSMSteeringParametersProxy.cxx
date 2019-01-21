/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCinemaDatabaseImporter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSteeringParametersProxy.h"

#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVTrivialProducer.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkSMSteeringParametersProxy);

class vtkSMSteeringParametersProxy::Internals
{
public:
  std::vector<vtkSmartPointer<vtkSMPropertyLink> > Links;
};

vtkSMSteeringParametersProxy::vtkSMSteeringParametersProxy()
  : P(new Internals)
{
}

vtkSMSteeringParametersProxy::~vtkSMSteeringParametersProxy()
{
  delete this->P;
}

void vtkSMSteeringParametersProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkSMSteeringParametersProxy::SetParameter(
  const char* parameter, vtkIdType tupleId, size_t numComps, double* values)
{
  if (!parameter)
  {
    return;
  }
  if (tupleId < 0)
  {
    return;
  }
  bool didSet = false;
  auto producer = dynamic_cast<vtkPVTrivialProducer*>(this->GetClientSideObject());
  if (producer)
  {
    auto dobj = producer->GetOutputDataObject(0);
    auto fields = dobj ? dobj->GetFieldData() : nullptr;
    vtkDataArray* arr = fields->GetArray(parameter);
    if (arr && arr->GetNumberOfComponents() == static_cast<int>(numComps) &&
      tupleId < arr->GetNumberOfTuples())
    {
      arr->SetTuple(tupleId, values);
      arr->Modified();
      didSet = true;
    }
  }
  if (!didSet)
  {
    vtkErrorMacro("Could not set property " << parameter);
  }
}

void vtkSMSteeringParametersProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();

  // Since links copy values from input to output,
  // we need to make sure that input properties i.e. the info
  // properties are not empty.
  this->UpdatePropertyInformation();

  vtkSMPropertyIterator* piter = this->NewPropertyIterator();
  for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
  {
    vtkSMProperty* prop = piter->GetProperty();
    vtkSMProperty* info = prop->GetInformationProperty();
    if (info)
    {
      // This ensures that the property value from the loaded state is
      // preserved, and not overwritten by the default value from
      // the property information
      info->Copy(prop);

      auto link = vtkSmartPointer<vtkSMPropertyLink>::New();

      // NOTE: vtkSMPropertyLink no longer affect proxy reference. We're now
      // only using vtkWeakPointer in vtkSMPropertyLink.
      link->AddLinkedProperty(this, piter->GetKey(), vtkSMLink::OUTPUT);
      link->AddLinkedProperty(this, this->GetPropertyName(info), vtkSMLink::INPUT);

      this->P->Links.push_back(link);
    }
  }
  piter->Delete();
}
