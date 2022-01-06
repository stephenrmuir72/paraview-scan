/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntRangeDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMIntRangeDomain
 * @brief
 *
 */

#ifndef vtkSMIntRangeDomain_h
#define vtkSMIntRangeDomain_h

#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkSMRangeDomainTemplate.h"

class VTKREMOTINGCORE_EXPORT vtkSMIntRangeDomain : public vtkSMRangeDomainTemplate<int>
{
public:
  static vtkSMIntRangeDomain* New();
  vtkTypeMacro(vtkSMIntRangeDomain, vtkSMRangeDomainTemplate<int>);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMIntRangeDomain();
  ~vtkSMIntRangeDomain() override;

private:
  vtkSMIntRangeDomain(const vtkSMIntRangeDomain&) = delete;
  void operator=(const vtkSMIntRangeDomain&) = delete;
};

#endif
