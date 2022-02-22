/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelayedSphereSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDelayedSphereSource.h"
#include "vtkLogger.h"
#include <chrono>
#include <thread>

vtkStandardNewMacro(vtkDelayedSphereSource);

//------------------------------------------------------------------------------
vtkDelayedSphereSource::vtkDelayedSphereSource()
  : Superclass()
{
  this->DataDelay = 0;
  this->InformationDelay = 0;
}

//------------------------------------------------------------------------------
int vtkDelayedSphereSource::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  auto now = std::chrono::high_resolution_clock::now();
  const auto end = now + std::chrono::duration<double>(this->DataDelay);

  double progress = 0.0;
  while (now < end)
  {
    std::this_thread::sleep_for(std::chrono::duration<double>(this->DataDelay * 0.1));
    now = std::chrono::high_resolution_clock::now();
    progress += 0.1;
    this->UpdateProgress(progress);
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
void vtkDelayedSphereSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RequestDataDelay: " << this->DataDelay << "\n";
  os << indent << "RequestInformationDelay: " << this->InformationDelay << "\n";
}

//------------------------------------------------------------------------------
int vtkDelayedSphereSource::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto now = std::chrono::high_resolution_clock::now();
  const auto end = now + std::chrono::duration<double>(this->InformationDelay);

  double progress = 0.0;
  while (now < end)
  {
    std::this_thread::sleep_for(std::chrono::duration<double>(this->InformationDelay * 0.1));
    now = std::chrono::high_resolution_clock::now();
    progress += 0.1;
    this->UpdateProgress(progress);
  }
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}
