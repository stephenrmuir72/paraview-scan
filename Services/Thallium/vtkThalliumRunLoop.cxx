/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumRunLoop.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThalliumRunLoop.h"

#include "vtkLogger.h"

//----------------------------------------------------------------------------
vtkThalliumRunLoop::vtkThalliumRunLoop(const std::string& name)
  : Pool(thallium::pool::create(thallium::pool::access::mpsc, thallium::pool::kind::fifo_wait))
  , XStream(thallium::xstream::create(thallium::scheduler::predef::basic_wait, *this->Pool))
{
  // Setup the rxcpp:run_loop on the thread associated with the XStream.
  this->XStream
    ->make_task([this, name]() {
      vtkLogger::SetThreadName(name);
      this->RunLoop.reset(new rxcpp::schedulers::run_loop());
      // Give the RxCpp run loop a function to let us schedule a wakeup in order to dispatch run
      // loop events
      this->RunLoop->set_notify_earlier_wakeup([this](auto const& /*when*/) {
        vtkLogF(TRACE, "notify_one");
        {
          std::unique_lock<thallium::mutex> lk(this->Mutex);
          this->SomethingEnqueued = true;
        }
        this->ConditionVariable.notify_one();
      });
    })
    ->join();

  this->ProcessEventsTask = this->XStream->make_thread([this, name]() {
    vtkLogF(INFO, "start run-loop");
    this->ProcessEvents();
    vtkLogF(INFO, "end run-loop");
    this->RunLoop.reset();
  });
}

//----------------------------------------------------------------------------
vtkThalliumRunLoop::~vtkThalliumRunLoop()
{
  try
  {
    this->WaitForExit();
  }
  catch (...)
  {
  }
}

//----------------------------------------------------------------------------
void vtkThalliumRunLoop::WaitForExit()
{
  std::unique_lock<thallium::mutex> lk(this->Mutex);
  if (this->Exit)
  {
    return;
  }
  this->Exit = true;
  lk.unlock();

  this->ConditionVariable.notify_one();
  this->ProcessEventsTask->join();
  this->XStream->join();
}

//----------------------------------------------------------------------------
void vtkThalliumRunLoop::ProcessEvents()
{
  const auto& runLoop = (*this->RunLoop);
  while (true)
  {
    while (!runLoop.empty()) // flush the runloop till all enqueued tasks are completed.
    {
      // dispatch tasks that are "ready"
      while (!runLoop.empty() && runLoop.peek().when < runLoop.now())
      {
        runLoop.dispatch();
      }

      // "sleep" for enqueued events not timed out yet.
      if (!runLoop.empty())
      {
        // if there are outstanding events, set the timer to wakeup for the first one
        const auto time_till_next_event = runLoop.peek().when - runLoop.now();
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time_till_next_event);
        const auto nseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
          time_till_next_event - std::chrono::duration_cast<std::chrono::milliseconds>(seconds));

        timespec interval;
        interval.tv_sec = seconds.count();
        interval.tv_nsec = nseconds.count();

        std::unique_lock<thallium::mutex> lk(this->Mutex);
        vtkLogF(TRACE, "wait-until");
        this->ConditionVariable.wait_until(lk, &interval);
        if (this->Exit)
        {
          return;
        }
      }
    }

    // now, we know the runLoop is empty, so we await a new task or exit.
    std::unique_lock<thallium::mutex> lk(this->Mutex);
    if (this->Exit)
    {
      return;
    }

    vtkLogF(TRACE, "wait");
    this->ConditionVariable.wait(lk, [this]() { return this->SomethingEnqueued || this->Exit; });
    this->SomethingEnqueued = false;
  }
}
