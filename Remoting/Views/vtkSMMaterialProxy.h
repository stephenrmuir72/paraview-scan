/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMMaterialProxy
 * @brief   proxy for a path tracer material types.
 *
 * This a proxy for controlling instances of materials in OSPRay
*/

#ifndef vtkSMMaterialProxy_h
#define vtkSMMaterialProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMMaterialProxy : public vtkSMProxy
{
public:
  static vtkSMMaterialProxy* New();
  vtkTypeMacro(vtkSMMaterialProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMMaterialProxy() = default;
  ~vtkSMMaterialProxy() override = default;

private:
  vtkSMMaterialProxy(const vtkSMMaterialProxy&) = delete;
  void operator=(const vtkSMMaterialProxy&) = delete;
};

#endif
