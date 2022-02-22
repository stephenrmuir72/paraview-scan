/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRxCPPBehavior.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLogger.h"

#include "vtk_rxcpp.h" // for rxcpp
// clang-format off
// ideally, we include rx-lite.hpp here.
#include VTK_REMOTING_RXCPP(rx.hpp)
// clang-format on

rxcpp::observable<int> GetObservable()
{
  using subject_t = rxcpp::subjects::behavior<int>;
  std::shared_ptr<subject_t> subject = std::make_shared<subject_t>(0);
  rxcpp::observable<>::just(100).subscribe([subject](int value) {
    vtkLogF(INFO, "pushing %d", value);
    auto subscriber = subject->get_subscriber();
    subscriber.on_next(value);
  });

  return subject->get_observable()
    .filter([](int value) { return value != 0; })
    .take(1)
    .map([subject](int value) {
      (void)subject;
      return value * 100;
    });
}

int TestRxCPPBehavior(int argc, char* argv[])
{
  auto observable = GetObservable();
  observable.subscribe([](int value) { vtkLogF(INFO, "received %d", value); });

  return EXIT_SUCCESS;
}
