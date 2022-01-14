/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInstantiator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInstantiator.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkObjectWrapper.h"
#include "vtkPVDataInformation.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringUtilities.h"

vtkStandardNewMacro(vtkSMInstantiator);
//----------------------------------------------------------------------------
vtkSMInstantiator::vtkSMInstantiator() = default;

//----------------------------------------------------------------------------
vtkSMInstantiator::~vtkSMInstantiator() = default;

//----------------------------------------------------------------------------
vtkSmartPointer<vtkObject> vtkSMInstantiator::CreateInstance(const std::string& className)
{
  if (className == "vtkSMProxy")
  {
    return vtk::TakeSmartPointer(vtkSMProxy::New());
  }
  else if (className == "vtkSMSourceProxy")
  {
    return vtk::TakeSmartPointer(vtkSMProxy::New());
  }
  else if (className == "vtkObjectWrapper")
  {
    return vtk::TakeSmartPointer(vtkObjectWrapper::New());
  }
  else if (className == "vtkSMProperty")
  {
    return vtk::TakeSmartPointer(vtkSMProperty::New());
  }
  else if (className == "vtkSMProperty")
  {
    return vtk::TakeSmartPointer(vtkSMProperty::New());
  }
  else if (className == "vtkSMIntVectorProperty")
  {
    return vtk::TakeSmartPointer(vtkSMIntVectorProperty::New());
  }
  else if (className == "vtkSMDoubleVectorProperty")
  {
    return vtk::TakeSmartPointer(vtkSMDoubleVectorProperty::New());
  }
  else if (className == "vtkSMIdTypeVectorProperty")
  {
    return vtk::TakeSmartPointer(vtkSMIdTypeVectorProperty::New());
  }
  else if (className == "vtkSMStringVectorProperty")
  {
    return vtk::TakeSmartPointer(vtkSMStringVectorProperty::New());
  }
  else if (className == "vtkSMInputProperty")
  {
    return vtk::TakeSmartPointer(vtkSMInputProperty::New());
  }
  else if (className == "vtkSMProxyProperty")
  {
    return vtk::TakeSmartPointer(vtkSMProxyProperty::New());
  }
  else if (className == "vtkSMDoubleRangeDomain")
  {
    return vtk::TakeSmartPointer(vtkSMDoubleRangeDomain::New());
  }
  else if (className == "vtkSMIntRangeDomain")
  {
    return vtk::TakeSmartPointer(vtkSMIntRangeDomain::New());
  }
  else if (className == "vtkPVDataInformation")
  {
    return vtk::TakeSmartPointer(vtkPVDataInformation::New());
  }

  vtkLogF(ERROR, "Unknown class type: %s", className.c_str());
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMInstantiator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
