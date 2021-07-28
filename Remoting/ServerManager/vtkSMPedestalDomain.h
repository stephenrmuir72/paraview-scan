/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPedestalDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMPedestalDomain_h
#define vtkSMPedestalDomain_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDoubleRangeDomain.h"

/**
 * @class vtkSMPedestalDomain
 * @brief Domain for Box and Cylinder Pedestal sources.
 *
 * Specialized domain for "Box Pedestal" and "Cylinder Pedestal" sources.
 * This domain initializes the pedestal property on which it operates based on
 * sources and filters that are visible in any view. It sets default properties
 * that place the Box or Cylinder pedestal so that it is just below the bounds
 * of all visible data data sources and filters, positioning it in the center
 * of the bounds in X and Z, setting the height to one quarter the bounds in Y,
 * and setting the radius/widths to the maximum of the bounds length in X or Z.
 *
 * This domain is very specialized for the "Box Pedestal" and "Cylinder Pedestal"
 * sources and should be used only with care with other sources.
 */
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPedestalDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMPedestalDomain* New();
  vtkTypeMacro(vtkSMPedestalDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMPedestalDomain();
  ~vtkSMPedestalDomain() override;

  // Overridden to calculate the Box/Cylinder default property values.
  int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) override;

private:
  vtkSMPedestalDomain(const vtkSMPedestalDomain&) = delete;
  void operator=(const vtkSMPedestalDomain&) = delete;
};

#endif // vtkSMPedestalDomain_h
