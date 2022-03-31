/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumRunLoop.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkThalliumRunLoop
 * @brief
 *
 */

#ifndef vtkThalliumRunLoop_h
#define vtkThalliumRunLoop_h

#include <memory>
#include <mutex>
#include <thallium.hpp>

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

class vtkThalliumRunLoop
{
  thallium::abt Scope;
  thallium::mutex Mutex;
  thallium::condition_variable ConditionVariable;
  thallium::managed<thallium::pool> Pool;
  thallium::managed<thallium::xstream> XStream;
  thallium::managed<thallium::thread> ProcessEventsTask;
  std::unique_ptr<rxcpp::schedulers::run_loop> RunLoop;
  bool Exit{ false };
  bool SomethingEnqueued{ false };

public:
  vtkThalliumRunLoop(const std::string& name);
  ~vtkThalliumRunLoop();

  void WaitForExit();
  const rxcpp::schedulers::run_loop& GetRunLoop() const { return *this->RunLoop; }

private:
  void ProcessEvents();

  std::chrono::milliseconds ms_until(
    rxcpp::schedulers::run_loop::clock_type::time_point const& when) const
  {
    const auto& runLoop = (*this->RunLoop);
    return (std::max)(
      ceil<std::chrono::milliseconds>(when - runLoop.now()), std::chrono::milliseconds::zero());
  }

  // Round the specified duration to the smallest number of `To` ticks that's not less than
  // `duration`
  template <class To, class Rep, class Period>
  static inline To ceil(const std::chrono::duration<Rep, Period>& duration)
  {
    const auto as_To = std::chrono::duration_cast<To>(duration);
    return (as_To < duration) ? (as_To + To{ 1 }) : as_To;
  }
};

#endif
