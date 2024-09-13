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
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMViewProxy.h"
#include "vtkSMSession.h"
#include "vtkSMTransferFunctionManager.h" // TO REMOVE

#include "vtkSMProxyClipboard.h"
#include "vtkSMProxyManager.h"

#include <string>

vtkStandardNewMacro(vtkSMBivariateTextureRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMBivariateTextureRepresentationProxy::vtkSMBivariateTextureRepresentationProxy()
{
  cout << "Bivariate Created" << endl;
  // this->SetDebug(true);
}

//----------------------------------------------------------------------------
vtkSMBivariateTextureRepresentationProxy::~vtkSMBivariateTextureRepresentationProxy()
{

}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::ViewUpdated(vtkSMProxy* view)
{
  this->CurrentView = view;
  this->Superclass::ViewUpdated(view);
}

// //----------------------------------------------------------------------------
// vtkSMProxy* vtkSMBivariateTextureRepresentationProxy::FindScalarBarRepresentation()
// {
//   if (!this->CurrentView || !this->CurrentView->GetProperty("Representations"))
//   {
//     vtkDebugMacro("No view");
//     return nullptr;
//   }

//   vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
//   if (!lutProperty) { cout << "no lut property" << endl; return nullptr; }
//   vtkSMPropertyHelper lutPropertyHelper(lutProperty);

//   vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();

//   vtkSMPropertyHelper reprHelper(this->CurrentView, "Representations");
//   for (unsigned int cc = 0; cc < reprHelper.GetNumberOfElements(); cc++)
//   {
//     vtkSMProxy* current = reprHelper.GetAsProxy(cc);
//     if (current && current->GetXMLName() &&
//       strcmp(current->GetXMLName(), "ScalarBarWidgetRepresentation2") == 0 &&
//       current->GetProperty("LookupTable"))
//     {
//       if (vtkSMPropertyHelper(current, "LookupTable").GetAsProxy() == this)
//       {
//         vtkDebugMacro("Can't retrieve LUT");
//         return current;
//       }
//     }
//   }
//   return nullptr;
// }


//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::CreateVTKObjects()
{
  cout << "Create VTK Objects" << endl;
    
  if (this->Initialized)
  {
    // vtkSMProperty* lutProperty = this->GetProperty("LookupTable");
    // if (!lutProperty) { cout << "no lut property" << endl; return; }
    // vtkSMPropertyHelper lutPropertyHelper(lutProperty);

    // vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
    // vtkSMScalarBarWidgetRepresentationProxy* oldSbProxy =
    //   vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
    //     vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, this->CurrentView));
    
    if (this->TexturedScalarBarProxy)
    {
      // Unregister previous textured scalar bar proxy.  vtkSMTransferFunctionManager::GetScalarBarRepresentation
      // will take care to create a new standard scalar bar when switching to other (internal) representations.
      cout << "Unregister previous proxy..." << endl;
      vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
      vtkSMSessionProxyManager* pxm = proxyManager->GetActiveSessionProxyManager();
      vtkSMPropertyHelper(this->CurrentView, "Representations").Remove(this->TexturedScalarBarProxy);
      this->CurrentView->UpdateVTKObjects();
      pxm->UnRegisterProxy(this->TexturedScalarBarProxy);
      this->TexturedScalarBarProxy = nullptr;

      // auto* view = vtkSMViewProxy::SafeDownCast(this->CurrentView);
      // if (view)
      // {
      //   cout << "Update View" << endl;
      //   view->GetSession()->PrepareProgress();
      //   view->StillRender();
      //   view->GetSession()->CleanupPendingProgress();
      // }
    }
    this->Initialized = false;
  }

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
void vtkSMBivariateTextureRepresentationProxy::Test(vtkObject*, unsigned long, void* vtkNotUsed(calldata))
{
  cout << "SUCESS" << endl;
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::OnPropertyUpdated(vtkObject*, unsigned long, void* vtkNotUsed(calldata))
{
  // XXX: see if when it's needed
  // this->InvalidateDataInformation();

  cout << "On property updated" << endl;
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

  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  vtkSMSessionProxyManager* pxm = proxyManager->GetActiveSessionProxyManager();

  vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy();
  vtkSMScalarBarWidgetRepresentationProxy* oldSbProxy =
    vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
      vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, this->CurrentView));
  
  if (oldSbProxy)
  {
    cout << "Unregister previous proxy..." << endl;
    vtkSMPropertyHelper(this->CurrentView, "Representations").Remove(oldSbProxy);
    pxm->UnRegisterProxy(oldSbProxy); 
    // oldSbProxy->Delete();
  }

  cout << "Creating new scalar bar proxy" << endl;
  vtkSMProxy* sbProxy = pxm->NewProxy("CustomScalarBarWidgetRepresentation", "ScalarBarWidgetRepresentation");
  if (!sbProxy)
  {
    vtkDebugMacro( << "Failed to create bar proxy." );
    return;
  }
  
  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(sbProxy);
  vtkSMPropertyHelper(sbProxy, "LookupTable").Set(lutProxy);
  controller->PostInitializeProxy(sbProxy);

  pxm->RegisterProxy("scalar_bars", sbProxy);
  this->TexturedScalarBarProxy = sbProxy;
  sbProxy->FastDelete();

  // vtkSMTransferFunctionManager::UpdateScalarBarsComponentTitle

  vtkSMPropertyHelper(this->CurrentView, "Representations").Add(sbProxy);
  this->CurrentView->UpdateVTKObjects();

  vtkSMProperty* sbTextureProperty = sbProxy->GetProperty("Texture");
  if (!sbTextureProperty)
  {
    vtkDebugMacro( << "No SB texture property" );
    return;
  }

  sbTextureProperty
    ->AddObserver(vtkCommand::ModifiedEvent, this,
      &vtkSMBivariateTextureRepresentationProxy::Test);

  // vtkSMProperty* sbActorProperty = sbProxy->GetProperty("ScalarBarActor");
  // if (!sbActorProperty)
  // {
  //   vtkDebugMacro(<< "Cannot retrieve ScalarBarActor property.");
  //   return;
  // }

  // vtkSmartPointer<vtkSMProxy> scalarBarActor2Proxy;
  // scalarBarActor2Proxy.TakeReference(pxm->NewProxy("annotations", "TexturedScalarBarActor"));
  // pxm->RegisterProxy("annotations", scalarBarActor2Proxy);

  // vtkSMPropertyHelper sbActorPropertyHelper(sbActorProperty);
  // vtkSMProxy* oldActorProxy = sbActorPropertyHelper.GetAsProxy();
  // if (!oldActorProxy)
  // {
  //   vtkDebugMacro(<< "Cannot retrieve old actor property !");
  //   return;
  // }

  // scalarBarActor2Proxy->Copy(oldActorProxy);
  // scalarBarActor2Proxy->UpdateVTKObjects();
  // // vtkNew<vtkSMProxyClipboard> clipboard;
  // // clipboard->Copy(oldActorProxy);
  // // clipboard->Paste(scalarBarActor2Proxy);

  // // vtkSMProperty* sbLUTProperty = oldActorProxy->GetProperty("LookupTable");
  // // if (!sbLUTProperty)
  // // {
  // //   vtkDebugMacro( << "Cannot retrieve old actor's LUT property." );
  // //   return;
  // // }
  // // vtkSMPropertyHelper sbLUTPropertyHelper(sbLUTProperty);
  // // vtkSMProxy* oldActorLUT = sbLUTPropertyHelper.GetAsProxy();
  // // if (!oldActorLUT)
  // // {
  // //   vtkDebugMacro( << "Cannot retrieve old actor LUT proxy !" );
  // //   return;
  // // }

  // // vtkSMProperty* newSbLUTProperty = scalarBarActor2Proxy->GetProperty("LookupTable");
  // // if (!newSbLUTProperty)
  // // {
  // //   vtkDebugMacro( << "Cannot retrieve new actor's LUT property." );
  // //   return;
  // // }
  // // vtkSMPropertyHelper newSbLUTPropertyHelper(newSbLUTProperty);
  // // newSbLUTPropertyHelper.Set(oldActorLUT);

  // sbActorPropertyHelper.Set(scalarBarActor2Proxy);
  // sbProxy->UpdateVTKObjects();

  vtkSMPropertyHelper sbTexturePropertyHelper(sbTextureProperty);
  // if (texturePropertyHelper.GetNumberOfElements() == 0 || !texturePropertyHelper.GetAsProxy())
  // {
  //   vtkDebugMacro( << "SB texture property helper fail." );
  //   return;
  // }

  // Set texture to scalar bar
  sbTexturePropertyHelper.Set(textureProxy);
  // sbProxy->UpdateVTKObjects();

  this->Initialized = false;
  this->UpdatePropertyInformation();
  this->Initialized = true;

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

  // // Need to manually think them - bad
  // scalarBarActor2Proxy->Copy(oldActorProxy);
  // scalarBarActor2Proxy->UpdateVTKObjects();

  sbProxy->UpdateVTKObjects();
  // scalarBarActor2Proxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMBivariateTextureRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
