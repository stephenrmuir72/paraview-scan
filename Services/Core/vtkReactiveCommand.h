/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReactiveCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkReactiveCommand
 * @brief  supports reactive events
 *
 * vtkReactiveCommand allows to bind vtkCommands to reactive events.
 */

#ifndef vtkReactiveCommand_h
#define vtkReactiveCommand_h

#include "vtkCommand.h"
#include "vtkServicesCoreModule.h" // for exports

#include "vtk_rxcpp.h"
// clang-format off
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

namespace rxvtk
{

/**
 * Given an object and an eventid create an observable based on this event.
 * The returned observable is of type `std::tuple<vtkObject*,unsigned long , void*>` which matches
 * the arguments of `vtkCommand::Execute`
 *
 * A common pattern is to attach a Subscriber to this call that registers what should happen each
 * time the event occurs. For example to create an observable based on the ModifiedEvent: <code>
 * vtkNew<vtkObject> object
 * auto observable = rxvtk::from_event(object,vtkCommand::ModifiedEvent);
 * // add a subscriber to this observable that is executed on each event
 * observable.subscribe([](rxvtk::ObservedType item)
 * {
 *    std::cout << "Event occurred "
 *  });
 *
 * // each modified event will cause the subscriber to execute its function.
 * object->Modified(); // outputs: "Event occured"
 * object->Modified(); // outputs: "Event occured"
 * </code>
 *
 * `observable.subscribe(...)` returns an `rxcpp::subscription`, by keeping track of the returned
 * object one can use it to unsubscribe from the observable. For example: <code> vtkNew<vtkObject>
 * object auto subscription = rxvtk::from_event(object,vtkCommand::ModifiedEvent)
 * .subscribe([](rxvtk::ObservedType item)
 * {
 *    std::cout << "Event occurred "
 *  });
 *
 * // each modified event will cause the subscriber to execute its function.
 * object->Modified(); // outputs: "Event occured"
 * subscription.unsubscribe();
 * object->Modified(); // no output
 * </code>
 */

using ObservedType = std::tuple<vtkObject*, unsigned long, void*>;

VTKSERVICESCORE_EXPORT rxcpp::observable<rxvtk::ObservedType> from_event(
  vtkObject* object, unsigned long eventId);
}

#endif /* vtkReactiveCommand_h */
