/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEventualImplementation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkEventualImplementation
 * @brief
 *
 */

#ifndef vtkEventualImplementation_h
#define vtkEventualImplementation_h

#include "vtkNJsonFwd.h" // for vtkNJson
#include "vtkObject.h"
#include "vtkServicesCoreModule.h" // for exports

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class VTKSERVICESCORE_EXPORT vtkEventualImplementation
{
public:
  vtkEventualImplementation(const vtkNJson& value);
  virtual ~vtkEventualImplementation();
  vtkEventualImplementation(const vtkEventualImplementation&) = delete;
  void operator=(const vtkEventualImplementation&) = delete;

  /**
   * Returns true if value is available.
   */
  virtual bool Test() const;

  /**
   * Returns value. This may block if value is not available yet.
   */
  virtual vtkNJson Wait() const;

  /**
   * Returns an observable.
   */
  rxcpp::observable<vtkNJson> GetObservable() const;

  /**
   * Set value.
   */
  virtual void SetValue(const vtkNJson& value) const;

private:
  rxcpp::subjects::behavior<vtkNJson> ValueBehavior;
};

#endif
