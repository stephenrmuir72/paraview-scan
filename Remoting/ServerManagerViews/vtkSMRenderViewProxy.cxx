/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRenderViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

vtkStandardNewMacro(vtkSMRenderViewProxy);
//----------------------------------------------------------------------------
vtkSMRenderViewProxy::vtkSMRenderViewProxy() {}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy::~vtkSMRenderViewProxy() {}

//----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::GetRepresentationType(vtkSMSourceProxy* producer, int outputPort)
{
  assert(producer);
  if (const char* reprName = this->Superclass::GetRepresentationType(producer, outputPort))
  {
    return reprName;
  }

  if (vtkPVXMLElement* hints = producer->GetHints())
  {
    // If the source has an hint as follows, then it's a text producer and must
    // be display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>
    for (unsigned int cc = 0, max = hints->GetNumberOfNestedElements(); cc < max; cc++)
    {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      const char* childName = child->GetName();
      const char* childType = child->GetAttribute("type");
      if (childName && strcmp(childName, "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) && index == outputPort && childType)
      {
        if (strcmp(childType, "text") == 0)
        {
          return "TextSourceRepresentation";
        }
        else if (strcmp(childType, "progress") == 0)
        {
          return "ProgressBarSourceRepresentation";
        }
        else if (strcmp(childType, "logo") == 0)
        {
          return "LogoSourceRepresentation";
        }
      }
    }
  }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  const char* representationsToTry[] = { "UnstructuredGridRepresentation",
    "StructuredGridRepresentation", "AMRRepresentation", "UniformGridRepresentation",
    "PVMoleculeRepresentation", "GeometryRepresentation", nullptr };
  for (int cc = 0; representationsToTry[cc] != nullptr; ++cc)
  {
    vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations", representationsToTry[cc]);
    if (prototype)
    {
      vtkSMProperty* inputProp = prototype->GetProperty("Input");
      vtkSMUncheckedPropertyHelper helper(inputProp);
      helper.Set(producer, outputPort);
      bool acceptable = (inputProp->IsInDomains() > 0);
      helper.SetNumberOfElements(0);
      if (acceptable)
      {
        return representationsToTry[cc];
      }
    }
  }

  // check if the data type is a vtkTable with a single row and column with
  // a vtkStringArray named "Text". If it is, we render this in a render view
  // with the value shown in the view.
  if (vtkSMOutputPort* port = producer->GetOutputPort(outputPort))
  {
    if (vtkPVDataInformation* dataInformation = port->GetDataInformation())
    {
      if (dataInformation->GetDataSetType() == VTK_TABLE)
      {
        if (vtkPVArrayInformation* ai =
              dataInformation->GetArrayInformation("Text", vtkDataObject::ROW))
        {
          if (ai->GetNumberOfComponents() == 1 && ai->GetNumberOfTuples() == 1)
          {
            return "TextSourceRepresentation";
          }
        }
      }
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
