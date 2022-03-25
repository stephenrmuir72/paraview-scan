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

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVTrackballEnvironmentRotate.h"
#include "vtkPVTrackballMultiRotate.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkPVTrackballZoomToMouse.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkTrackballPan.h"

class vtkSMRenderViewProxy::vtkInternals
{
  vtkNew<vtkPVInteractorStyle> Style3D;
  vtkNew<vtkPVInteractorStyle> Style2D;
  std::map<int, vtkSmartPointer<vtkCameraManipulator>> Manipulators;

public:
  vtkInternals()
  {
    this->Manipulators[vtkSMRenderViewProxy::PAN] = vtk::TakeSmartPointer(vtkTrackballPan::New());
    this->Manipulators[vtkSMRenderViewProxy::ZOOM] =
      vtk::TakeSmartPointer(vtkPVTrackballZoom::New());
    this->Manipulators[vtkSMRenderViewProxy::ROLL] =
      vtk::TakeSmartPointer(vtkPVTrackballRoll::New());
    this->Manipulators[vtkSMRenderViewProxy::ROTATE] =
      vtk::TakeSmartPointer(vtkPVTrackballRotate::New());
    this->Manipulators[vtkSMRenderViewProxy::MULTI_ROTATE] =
      vtk::TakeSmartPointer(vtkPVTrackballMultiRotate::New());
    this->Manipulators[vtkSMRenderViewProxy::ZOOM_TO_MOUSE] =
      vtk::TakeSmartPointer(vtkPVTrackballZoomToMouse::New());
    this->Manipulators[vtkSMRenderViewProxy::SKYBOX_ROTATE] =
      vtk::TakeSmartPointer(vtkPVTrackballEnvironmentRotate::New());
  }

  void SetCenterOfRotation(double center[3])
  {
    this->Style2D->SetCenterOfRotation(center);
    this->Style3D->SetCenterOfRotation(center);
  }

  void SetRotationFactor(double factor)
  {
    this->Style2D->SetRotationFactor(factor);
    this->Style3D->SetRotationFactor(factor);
  }

  void SetCamera2DMouseWheelMotionFactor(double factor)
  {
    this->Style2D->SetMouseWheelMotionFactor(factor);
  }

  void SetCamera3DMouseWheelMotionFactor(double factor)
  {
    this->Style3D->SetMouseWheelMotionFactor(factor);
  }

  void SetInteractionMode(int mode, vtkRenderWindowInteractor* iren)
  {
    switch (mode)
    {
      case vtkSMRenderViewProxy::INTERACTION_MODE_2D:
        iren->SetInteractorStyle(this->Style2D);
        break;

      case vtkSMRenderViewProxy::INTERACTION_MODE_3D:
      default:
        iren->SetInteractorStyle(this->Style3D);
        break;
    }
  }

  void UpdateInteractionManipulators(const char* name, vtkSMRenderViewProxy* view, bool twoD)
  {
    auto* prop = view->GetProperty(name);
    if (prop && !twoD)
    {
      int manipulators[9];
      vtkSMPropertyHelper(prop).Get(manipulators, 9);
      this->SetInteractionManipulators(this->Style3D, manipulators);
    }
    if (prop && twoD)
    {
      int manipulators[9];
      vtkSMPropertyHelper(prop).Get(manipulators, 9);
      this->SetInteractionManipulators(this->Style2D, manipulators);
    }
  }

  void SetInteractionManipulators(vtkPVInteractorStyle* style, const int manipulators[9])
  {
    if (!style)
    {
      return;
    }
    style->RemoveAllManipulators();

    enum
    {
      NONE = 0,
      SHIFT = 1,
      CTRL = 2
    };

    enum
    {
      PAN = 1,
      ZOOM = 2,
      ROLL = 3,
      ROTATE = 4,
      MULTI_ROTATE = 5,
      ZOOM_TO_MOUSE = 6,
      SKYBOX_ROTATE = 7
    };

    for (int manip = vtkSMRenderViewProxy::NONE; manip <= vtkSMRenderViewProxy::CTRL; manip++)
    {
      for (int button = 0; button < 3; button++)
      {
        int manipType = manipulators[3 * manip + button];
        if (auto& type = this->Manipulators[manipType])
        {
          auto cameraManipulator = vtk::TakeSmartPointer(type->NewInstance());
          cameraManipulator->SetButton(button + 1); // since button starts with 1.
          cameraManipulator->SetControl(manip == CTRL ? 1 : 0);
          cameraManipulator->SetShift(manip == SHIFT ? 1 : 0);
          style->AddManipulator(cameraManipulator);
        }
      }
    }
  }
};

vtkStandardNewMacro(vtkSMRenderViewProxy);
//----------------------------------------------------------------------------
vtkSMRenderViewProxy::vtkSMRenderViewProxy()
  : Internals(new vtkSMRenderViewProxy::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy::~vtkSMRenderViewProxy() = default;

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
void vtkSMRenderViewProxy::InitializeInteractor(vtkRenderWindowInteractor* iren)
{
  this->Superclass::InitializeInteractor(iren);

  auto& internals = (*this->Internals);
  internals.SetInteractionMode(vtkSMPropertyHelper(this, "InteractionMode").GetAsInt(), iren);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  this->Superclass::SetPropertyModifiedFlag(name, flag);
  if (this->DoNotModifyProperty || name == nullptr)
  {
    return;
  }

  auto& internals = (*this->Internals);
  if (strcmp(name, "InteractionMode") == 0)
  {
    if (auto* iren = this->GetRenderWindowInteractor())
    {
      internals.SetInteractionMode(vtkSMPropertyHelper(this, name).GetAsInt(), iren);
    }
  }
  else if (strcmp(name, "Camera2DManipulators") == 0)
  {
    internals.UpdateInteractionManipulators(name, this, true);
  }
  else if (strcmp(name, "Camera3DManipulators") == 0)
  {
    internals.UpdateInteractionManipulators(name, this, false);
  }
  else if (strcmp(name, "CenterOfRotation") == 0)
  {
    double center[3];
    vtkSMPropertyHelper(this, name).Get(center, 3);
    internals.SetCenterOfRotation(center);
  }
  else if (strcmp(name, "RotationFactor") == 0)
  {
    internals.SetRotationFactor(vtkSMPropertyHelper(this, name).GetAsDouble());
  }
  else if (strcmp(name, "Camera3DMouseWheelMotionFactor") == 0)
  {
    internals.SetCamera3DMouseWheelMotionFactor(vtkSMPropertyHelper(this, name).GetAsDouble());
  }
  else if (strcmp(name, "Camera2DMouseWheelMotionFactor") == 0)
  {
    internals.SetCamera2DMouseWheelMotionFactor(vtkSMPropertyHelper(this, name).GetAsDouble());
  }
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
