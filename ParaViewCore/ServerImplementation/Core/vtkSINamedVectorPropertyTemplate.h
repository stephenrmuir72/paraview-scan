/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSINamedVectorPropertyTemplate
 *
 *
*/

#ifndef vtkSINamedVectorPropertyTemplate_h
#define vtkSINamedVectorPropertyTemplate_h

#include "vtkSIVectorPropertyTemplate.h"

#include <string>

template <class T, class force_idtype = int>
class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSINamedVectorPropertyTemplate
  : public vtkSIVectorPropertyTemplate<T, force_idtype>
{
public:
  typedef vtkSIVectorPropertyTemplate<T, force_idtype> Superclass;
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSINamedVectorPropertyTemplate();
  ~vtkSINamedVectorPropertyTemplate() override;

  /**
   * Pull the current state of the underneath implementation
   */
  bool Pull(vtkSMMessage*) override;

  /**
   * Parse the xml for the property.
   */
  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) override;

  /**
   * Implements the actual push.
   */
  bool Push(T* values, int number_of_elements) override;

private:
  vtkSINamedVectorPropertyTemplate(const vtkSINamedVectorPropertyTemplate&) = delete;
  void operator=(const vtkSINamedVectorPropertyTemplate&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkSINamedVectorPropertyTemplate.h
