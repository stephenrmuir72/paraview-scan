/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReactiveCommand.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkReactiveCommand.h"
#include <tuple>             // for std::tuple
#include <vtkSmartPointer.h> // for vtk::TakeSmartPointer

class vtkReactiveCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkReactiveCommand, vtkCommand);

  vtkReactiveCommand(const vtkReactiveCommand&) = delete;
  vtkReactiveCommand& operator=(const vtkReactiveCommand&) = delete;

  static vtkReactiveCommand* New() { return new vtkReactiveCommand(); }

  /**
   * Satisfy the superclass API for callbacks. Recall that the caller is
   * the instance invoking the event; eid is the event id (see
   * vtkCommand.h); and calldata is information sent when the callback
   * was invoked (e.g., progress value in the vtkCommand::ProgressEvent).
   */
  void Execute(vtkObject* caller, unsigned long eventId, void* callData) override;

  using SubscriberType = rxcpp::subscriber<rxvtk::ObservedType>;

  /**
   * Subscriber that will be fired on each `Execute` call
   */
  SubscriberType Subscriber;

  /**
   * Keep track of observed objkect to avoid accessing dabling pointer
   */
  bool ObjectIsDeleted{ false };

  /**
   * Id of the event this command is listening to
   */
  unsigned long EventId;

protected:
  vtkReactiveCommand();
  ~vtkReactiveCommand() override;
};

//---------------------------------------------------------------------------
vtkReactiveCommand::vtkReactiveCommand()
  : Subscriber(
      rxcpp::make_subscriber<rxvtk::ObservedType>(rxcpp::make_observer<rxvtk::ObservedType>()))
{
}

//---------------------------------------------------------------------------
vtkReactiveCommand::~vtkReactiveCommand() = default;

//---------------------------------------------------------------------------
void vtkReactiveCommand::Execute(vtkObject* caller, unsigned long eventId, void* callData)
{
  if (eventId == this->EventId)
  {
    this->Subscriber.on_next(std::make_tuple(caller, eventId, callData));
  }

  if (eventId == vtkCommand::DeleteEvent)
  {
    this->ObjectIsDeleted = true;
    this->Subscriber.on_completed();
  }
}

//===========================================================================
namespace rxvtk
{
rxcpp::observable<rxvtk::ObservedType> from_event(vtkObject* object, unsigned long eventId)
{
  if (!object)
  {
    return rxcpp::sources::never<rxvtk::ObservedType>();
  }

  return rxcpp::observable<>::create<rxvtk::ObservedType>(
    [object, eventId](const vtkReactiveCommand::SubscriberType& s) {
      auto command = vtk::TakeSmartPointer(vtkReactiveCommand::New());
      command->Subscriber = s;
      command->EventId = eventId;
      const unsigned long tag1 = object->AddObserver(eventId, command);
      const unsigned long tag2 = object->AddObserver(vtkCommand::DeleteEvent, command);

      // add a function to be executed upon unsubscription
      s.add([tag1, tag2, object, command]() {
        if (!command->ObjectIsDeleted)
        {
          object->RemoveObserver(tag1);
          object->RemoveObserver(tag2);
        }
      });
    });
}
}
