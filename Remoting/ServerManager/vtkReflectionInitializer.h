/*=========================================================================

  Program:   ParaView
  Module:    vtkReflectionInitializer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkReflectionInitializer
 * @brief
 *
 */

#ifndef vtkReflectionInitializer_h
#define vtkReflectionInitializer_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" // for exports

class VTKREMOTINGSERVERMANAGER_EXPORT vtkReflectionInitializer : public vtkObject
{
public:
  static vtkReflectionInitializer* New();
  vtkTypeMacro(vtkReflectionInitializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void Initialize() = 0;

protected:
  vtkReflectionInitializer();
  ~vtkReflectionInitializer();

private:
  vtkReflectionInitializer(const vtkReflectionInitializer&) = delete;
  void operator=(const vtkReflectionInitializer&) = delete;
};

#endif
