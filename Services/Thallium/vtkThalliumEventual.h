/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumEventual.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkThalliumEventual_h
#define vtkThalliumEventual_h

#include "vtkEventual.h"
#include "vtkRemotingThalliumModule.h" // for exports
#include <thallium.hpp>                // for thallium::eventual

/*
 * Internal class. Meant for use within this module alone.
 */
template <typename T>
class vtkThalliumEventualImpl : public vtkEventualImpl<T>
{
public:
  vtkThalliumEventualImpl() = default;
  ~vtkThalliumEventualImpl() override = default;
  vtkThalliumEventualImpl(const vtkThalliumEventualImpl<T>&) = delete;
  void operator=(const vtkThalliumEventualImpl<T>&) = delete;

  bool Test() override { return this->Eventual.test(); }
  T Wait() override { return this->Eventual.wait(); }
  void SetValue(const T& value) override { this->Eventual.set_value(value); }

private:
  thallium::eventual<T> Eventual;
};

template <typename T>
class vtkThalliumAsyncResponseImpl : public vtkEventualImpl<T>
{
public:
  vtkThalliumAsyncResponseImpl() = default;
  ~vtkThalliumAsyncResponseImpl() override = default;
  vtkThalliumAsyncResponseImpl(const vtkThalliumAsyncResponseImpl<T>&) = delete;
  void operator=(const vtkThalliumAsyncResponseImpl<T>&) = delete;

  vtkThalliumAsyncResponseImpl(thallium::async_response&& response)
    : Response(std::move(response))
  {
  }

  bool Test() override { return this->Response.received(); }
  T Wait() override { return this->Response.wait(); }
  void SetValue(const T& value) override { throw std::runtime_error("call not supported!"); }

private:
  thallium::async_response Response;
};
#endif
