/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMDoubleRangeDomain
 * @brief
 *
 */

#ifndef vtkSMDoubleRangeDomain_h
#define vtkSMDoubleRangeDomain_h

#include "vtkSMRangeDomainTemplate.h"
#include "vtkServerManagerCoreModule.h" //needed for exports

class VTKSERVERMANAGERCORE_EXPORT vtkSMDoubleRangeDomain : public vtkSMRangeDomainTemplate<double>
{
public:
  static vtkSMDoubleRangeDomain* New();
  vtkTypeMacro(vtkSMDoubleRangeDomain, vtkSMRangeDomainTemplate<double>);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMDoubleRangeDomain();
  ~vtkSMDoubleRangeDomain() override;

private:
  vtkSMDoubleRangeDomain(const vtkSMDoubleRangeDomain&) = delete;
  void operator=(const vtkSMDoubleRangeDomain&) = delete;
};

#endif
