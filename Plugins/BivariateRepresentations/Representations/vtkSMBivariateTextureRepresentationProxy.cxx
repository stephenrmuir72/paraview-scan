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
#include "vtkSMLink.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMSessionProxyManager.h"
#include "vtksys/SystemTools.hxx"

#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"

#include <string>

vtkStandardNewMacro(vtkSMBivariateTextureRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMBivariateTextureRepresentationProxy::vtkSMBivariateTextureRepresentationProxy()
{
  // this->SetDebug(true);
}

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

  // this->SetupPropertiesLinks();

  // XXX : look for property links and do everything on proxy side
  this->AddObserver(
    vtkCommand::UpdateDataEvent, this, &vtkSMBivariateTextureRepresentationProxy::OnPropertyUpdated);
  // this->AddObserver(
  //   vtkCommand::UpdateDataEvent, this, &vtkSMBivariateTextureRepresentationProxy::SetupPropertiesLinks);
}

//-----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::SetupPropertiesLinks()
{
  // Since links copy values from input to output,
  // we need to make sure that input properties i.e. the info
  // properties are not empty.
  this->UpdatePropertyInformation();

  // Clean current property links
  this->Links.clear();

  // If there is no view, we will not have any scalar bar so we 
  // don't need to go further.
  if (!this->CurrentView)
  {
    cout << "no current view" << endl;
    return;
  }

  // Retrieve the lut property of the bivariate texture representation
  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty) { cout << "no lut property" << endl; return; }
  vtkSMPropertyHelper lutPropertyHelper(lutProperty);

  // Retrieve the scalar bar proxy given the lut and the active view
  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  if (!lutProxy) { cout << "no lut proxy" << endl; return; }
  vtkSMScalarBarWidgetRepresentationProxy* sbProxy =
    vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, this->CurrentView));
  if (!sbProxy) { cout << "no sb proxy" << endl; return; }

  // Link the texture property of the representation with the texture
  // property of the scalar bar
  vtkSMPropertyLink* link = vtkSMPropertyLink::New();
  link->AddLinkedProperty(this, "BivariateTexture", vtkSMLink::INPUT);
  link->AddLinkedProperty(sbProxy, "Texture", vtkSMLink::OUTPUT);
  this->Links.push_back(link);
  link->Delete();
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::OnPropertyUpdated(vtkObject*, unsigned long, void* vtkNotUsed(calldata))
{
  // XXX: see if when it's needed
  // this->InvalidateDataInformation();

  vtkSMProperty* textureProperty = this->GetProperty("BivariateTexture");

  if (!textureProperty)
  {
    vtkDebugMacro( << "No texture property." );
    return;
  }

  vtkSMPropertyHelper texturePropertyHelper(textureProperty);
  vtkSMProxy* textureProxy = texturePropertyHelper.GetAsProxy();
  if (!textureProxy)
  {
    vtkDebugMacro( << "No texture proxy." );
    return;
  }

  vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
  if (!lutProperty)
  {
    vtkDebugMacro( << "No LUT property." );
    return;
  }

  vtkSMPropertyHelper lutPropertyHelper(lutProperty);
  if (lutPropertyHelper.GetNumberOfElements() == 0 || !lutPropertyHelper.GetAsProxy())
  {
    vtkDebugMacro( << "LUT property helper fail." );
    return;
  }


  if (!this->CurrentView)
  {
    vtkDebugMacro( << "No current view." );
    return;
  }

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  vtkSMScalarBarWidgetRepresentationProxy* sbProxy =
    vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, this->CurrentView));

  if (!sbProxy)
  {
    vtkDebugMacro( << "No scalar bar proxy." );
    return;
  }

  vtkSMProperty* sbTextureProperty = sbProxy->GetProperty("Texture");
  if (!sbTextureProperty)
  {
    vtkDebugMacro( << "No SB texture property" );
    return;
  }

  vtkSMPropertyHelper sbTexturePropertyHelper(sbTextureProperty);
  // if (texturePropertyHelper.GetNumberOfElements() == 0 || !texturePropertyHelper.GetAsProxy())
  // {
  //   vtkDebugMacro( << "SB texture property helper fail." );
  //   return;
  // }

  // Set texture to scalar bar
  sbTexturePropertyHelper.Set(textureProxy);
  sbProxy->UpdateVTKObjects();

  this->UpdatePropertyInformation();

  // Now take care of axis for 1st and 2nd array
  vtkSMProperty* firstArray = this->GetProperty("FirstArrayRange");
  if (!firstArray)
  {
    vtkDebugMacro( << "No first array range." );
    return;
  }
  vtkSMPropertyHelper firstArrayHelper(firstArray);
  double range1[2] = { 0, 0 };
  firstArrayHelper.Get(range1, 2);
  vtkDebugMacro( << "Range 1 : " << range1[0] << " " << range1[1] );

  vtkSMProperty* secondArray = this->GetProperty("SecondArrayRange");
  if (!secondArray)
  {
    vtkDebugMacro( << "No second array range." );
    return;
  }
  vtkSMPropertyHelper secondArrayHelper(secondArray);
  double range2[2] = { 0, 0 };
  secondArrayHelper.Get(range2, 2);
  vtkDebugMacro( << "Range 2 : " << range2[0] << " " << range2[1] );

  vtkSMProperty* firstRange = sbProxy->GetProperty("FirstRange");
  if (!firstRange)
  {
    vtkDebugMacro( << "No first range." );
    return;
  }
  vtkSMPropertyHelper firstRangeHelper(firstRange);
  firstRangeHelper.Set(range1, 2);

  vtkSMProperty* secondRange = sbProxy->GetProperty("SecondRange");
  if (!secondRange)
  {
    vtkDebugMacro( << "No second range." );
    return;
  }
  vtkSMPropertyHelper secondRangeHelper(secondRange);
  secondRangeHelper.Set(range2, 2);

  // Now take care of titles for 1st and 2nd array
  vtkSMProperty* firstArrayName = this->GetProperty("FirstArrayName");
  // this->UpdatePropertyInformation(firstArrayName);
  if (!firstArrayName)
  {
    vtkDebugMacro( << "No first array name." );
    return;
  }
  vtkSMPropertyHelper firstArrayNameHelper(firstArrayName);
  
  if (!firstArrayNameHelper.GetAsString())
  {
    vtkDebugMacro( << "No first array name as string." );
    return;
  }
  std::string name1(firstArrayNameHelper.GetAsString());
  vtkDebugMacro( << "Name 1 : " << name1 );

  vtkSMProperty* secondArrayName = this->GetProperty("SecondArrayName");
  if (!secondArrayName)
  {
    vtkDebugMacro( << "No second array name." );
    return;
  }
  vtkSMPropertyHelper secondArrayNameHelper(secondArrayName);
  if (!secondArrayNameHelper.GetAsString())
  {
    vtkDebugMacro( << "No second array name as string." );
    return;
  }
  std::string name2(secondArrayNameHelper.GetAsString());
  vtkDebugMacro( << "Name 2 : " << name2 );

  vtkSMProperty* sbFirstName = sbProxy->GetProperty("Title");
  if (!sbFirstName)
  {
    vtkDebugMacro( << "No Title 1" );
    return;
  }
  vtkSMPropertyHelper sbFirstNameHelper(sbFirstName);
  sbFirstNameHelper.Set(name1.c_str());

  vtkSMProperty* sbSecondName = sbProxy->GetProperty("Title2");
  if (!sbSecondName)
  {
    vtkDebugMacro( << "No Title 2" );
    return;
  }
  vtkSMPropertyHelper sbSecondNameHelper(sbSecondName);
  sbSecondNameHelper.Set(name2.c_str());

  // firstArrayName->AddLinkedProperty(sbSecondName);
  // secondArrayName->AddLinkedProperty(sbSecondName);
  // this->UpdatePropertyInformation(); // ?

  sbProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
