/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEventual.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkEventual
 * @brief a type whose value gets set concurrently
 *
 * vtkEventual is similar to a `std::future`. Its value will get set at some
 * point in the future concurrently. It provides API to test is the value is
 * available and wait-and-fetch the value.
 *
 * Typical usage:
 *
 * @code{cpp}
 *
 * vtkServiceEndpoint *ep = ...
 * vtkEventual<vtkSession::ConnectionStatus> status = ep->Connect(...);
 *
 * // to check is connection status is available:
 * bool ready = status.Test();
 *
 * // to get status, may block if status is not yet available.
 * vtkSession::ConnectionStatus result = status.Wait();
 *
 * @subsection ImplementationDetails Implementation Details
 *
 * Concrete implementation provides should subclass vtkEventualImpl.
 * Subsequently, use `vtkEventual(std::shared_ptr<vtkEventualImpl<T>> impl)` to
 * construct the eventual providing it with the implementation-specific
 * instance.
 *
 * @endcode
 */

#ifndef vtkEventual_h
#define vtkEventual_h

#include "vtkEventualImplementation.h" // for vtkEventualImplementation
#include "vtkJson.h"                   // for vtkJson
#include "vtkServicesCoreModule.h"     // for exports

#include <memory> // for std::shared_ptr

template <typename T>
class VTKSERVICESCORE_EXPORT vtkEventual
{
  std::shared_ptr<vtkEventualImplementation> Internals;

public:
  vtkEventual() = default;
  vtkEventual(std::shared_ptr<vtkEventualImplementation> impl)
    : Internals(std::move(impl))
  {
  }
  vtkEventual(const T& value)
    : Internals(new vtkEventualImplementation(vtkJson(value)))
  {
  }
  ~vtkEventual() = default;

  static vtkEventual<T> Empty() { return vtkEventual<T>{}; }

  /**
   * Returns true if the value is available.
   */
  bool Test() { return this->Internals ? this->Internals->Test() : false; }

  /**
   * Returns the contained value. May block if the value is not yet available.
   */
  T Wait() const
  {
    if (this->Internals)
    {
      const auto var = this->Internals->Wait();
      return var.get<T>();
    }
    return T{};
  }

  /**
   * Set the value.
   */
  void SetValue(const T& value) const
  {
    if (this->Internals)
    {
      this->Internals->SetValue(value);
    }
  }

  /**
   * Returns an observable for the eventual's value. The observable will emit at
   * most 1 value.
   */
  rxcpp::observable<T> GetObservable() const
  {
    if (!this->Internals)
    {
      return rxcpp::sources::empty<T>();
    }
    return this->Internals->GetObservable().map(
      [](const vtkJson& value) { return value.get<T>(); });
  }
};
#endif
