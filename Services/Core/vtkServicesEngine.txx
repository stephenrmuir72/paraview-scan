/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkServicesEngine.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkServicesEngine_txx
#define vtkServicesEngine_txx

//-----------------------------------------------------------------------------
template <typename Rep, typename Period>
inline void vtkServicesEngine::ProcessEventsFor(const std::chrono::duration<Rep, Period>& duration)
{
  const auto start = std::chrono::system_clock::now();
  do
  {
    this->ProcessEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  } while (duration > (std::chrono::system_clock::now() - start));
}

//-----------------------------------------------------------------------------
template <typename Rep, typename Period, typename Predicate>
inline bool vtkServicesEngine::ProcessEventsFor(
  const std::chrono::duration<Rep, Period>& duration, Predicate stopProcessing)
{
  const auto start = std::chrono::system_clock::now();
  do
  {
    this->ProcessEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  } while (!stopProcessing() && duration > (std::chrono::system_clock::now() - start));
  return stopProcessing();
}

#endif
