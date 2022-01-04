/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThalliumEventualImplementation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkThalliumEventualImplementation
 * @brief
 *
 */

#ifndef vtkThalliumEventualImplementation_h
#define vtkThalliumEventualImplementation_h

#include "vtkEventualImplementation.h"
#include "vtkJson.h"                   // for vtkJson
#include "vtkServicesThalliumModule.h" // for exports

#include <thallium.hpp>

class VTKSERVICESTHALLIUM_EXPORT vtkThalliumEventualImplementation
  : public vtkEventualImplementation
{
  mutable thallium::eventual<vtkJson> Eventual;

public:
  vtkThalliumEventualImplementation();
  bool Test() const override;
  void SetValue(const vtkJson& value) const override;
  vtkJson Wait() const override;
};

#endif
