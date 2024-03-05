// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMBivariateTextureRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtksys/SystemTools.hxx"

#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"

#include <string>

vtkStandardNewMacro(vtkSMBivariateTextureRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMBivariateTextureRepresentationProxy::vtkSMBivariateTextureRepresentationProxy() = default;

//----------------------------------------------------------------------------
vtkSMBivariateTextureRepresentationProxy::~vtkSMBivariateTextureRepresentationProxy() = default;

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::ViewUpdated(vtkSMProxy* view)
{
  this->CurrentView = view;
  this->Superclass::ViewUpdated(view);
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
  {
    return;
  }

  // Ensure that we update the RepresentationTypesInfo property and the domain
  // for "Representations" property before CreateVTKObjects() is finished. This
  // ensure that all representations have valid Representations domain.
  this->UpdatePropertyInformation();

  // Whenever the "Representation" property is modified, we ensure that the
  // this->InvalidateDataInformation() is called.
  this->AddObserver(
    vtkCommand::UpdatePropertyEvent, this, &vtkSMBivariateTextureRepresentationProxy::OnPropertyUpdated);
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::OnPropertyUpdated(vtkObject*, unsigned long, void* calldata)
{
  const char* pname = reinterpret_cast<const char*>(calldata);
  if (pname && strcmp(pname, "Representation") == 0)
  {
    this->InvalidateDataInformation();
  }

  vtkSMProperty* textureProperty = this->GetProperty("Texture");
  if (!textureProperty)
  {
    cout << "No texture property." << endl;
    return;
  }

  vtkSMPropertyHelper texturePropertyHelper(textureProperty);
  vtkSMProxy* textureProxy = texturePropertyHelper.GetAsProxy();
  if (!textureProxy)
  {
    cout << "No texture proxy." << endl;
    return;
  }

  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty)
  {
    cout << "No LUT property." << endl;
    return;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  // if (lutPropertyHelper.GetNumberOfElements() == 0 || !lutPropertyHelper.GetAsProxy())
  // {
  //   cout << "LUT property helper fail." << endl;
  //   return;
  // }


  if (!this->CurrentView)
  {
    cout << "No current view." << endl;
    return;
  }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  vtkSMScalarBarWidgetRepresentationProxy* sbProxy =
    vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, this->CurrentView));

  if (!sbProxy)
  {
    cout << "No scalar bar proxy." << endl;
    return;
  }

  vtkSMProperty* sbTextureProperty = sbProxy->GetProperty("Texture");
  if (!sbTextureProperty)
  {
    cout << "No SB texture property" << endl;
    return;
  }

  vtkSMPropertyHelper sbTexturePropertyHelper(sbTextureProperty);
  // if (texturePropertyHelper.GetNumberOfElements() == 0 || !texturePropertyHelper.GetAsProxy())
  // {
  //   cout << "SB texture property helper fail." << endl;
  //   return;
  // }

  // Set texture to scalar bar
  sbTexturePropertyHelper.Set(textureProxy);
  sbProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
