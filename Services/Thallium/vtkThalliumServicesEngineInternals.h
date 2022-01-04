/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumServicesEngineInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkThalliumServicesEngineInternals_h
#define vtkThalliumServicesEngineInternals_h

#include <map>
#include <memory> // for std::unique_ptr
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <vtkSmartPointer.h>

class vtkMultiProcessController;

class vtkThalliumServicesEngineInternals
{
  vtkSmartPointer<vtkMultiProcessController> Controller;
  std::unique_ptr<thallium::abt> ArgobotsScope;

  thallium::managed<thallium::pool> RPCPool;
  thallium::managed<thallium::xstream> RPCXstream;

  thallium::managed<thallium::pool> ProgressPool;
  thallium::managed<thallium::xstream> ProgressXstream;

  std::unique_ptr<thallium::engine> Engine;
  std::map<std::string, uint16_t> ProviderIds;
  thallium::mutex ProviderIdsMutex;
  uint16_t NextProviderId{ 1 };

public:
  /**
   * Initializes the Thallium environment. On root node, the thallium engine is
   * initialized for RPC. On satellites, the engine is not intended for use and
   * is only initialized for consistency.
   */
  vtkThalliumServicesEngineInternals(const std::string& url, vtkMultiProcessController* controller);
  ~vtkThalliumServicesEngineInternals();

  /**
   * Returns the engine URL.
   *
   * This call is threadsafe.
   */
  std::string GetUrl() const { return this->Engine->self(); }

  /**
   * Returns the engine.
   *
   * This call is threadsafe.
   */
  thallium::engine GetEngine() { return (*this->Engine); };

  /**
   * Returns the provider id for a specific service name. Will assign a new one
   * if none exists.
   *
   * This call is threadsafe.
   */
  uint16_t GetProviderId(const std::string& serviceName);
};

#endif
