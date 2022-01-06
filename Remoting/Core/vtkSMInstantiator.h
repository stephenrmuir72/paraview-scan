/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInstantiator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMInstantiator
 * @brief
 *
 */

#ifndef vtkSMInstantiator_h
#define vtkSMInstantiator_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" // for exports
#include "vtkSmartPointer.h"       // for vtkSmartPointer

class VTKREMOTINGCORE_EXPORT vtkSMInstantiator : public vtkObject
{
public:
  static vtkSMInstantiator* New();
  vtkTypeMacro(vtkSMInstantiator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates an instance of the specified class.
   */
  static vtkSmartPointer<vtkObject> CreateInstance(const std::string& className);

protected:
  vtkSMInstantiator();
  ~vtkSMInstantiator();

private:
  vtkSMInstantiator(const vtkSMInstantiator&) = delete;
  void operator=(const vtkSMInstantiator&) = delete;
};

#endif
