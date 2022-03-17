/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMMaterialDomain
 * @brief lists available materials
 *
 * vtkSMMaterialDomain is intended for use with vtkSMStringVectorProperty
 * instances that should allow the user to choose a material. The domai
 * populates itself with materials known to the session specific material
 * library, if one exists when the domain is created.
 *
 * When ParaView has no materials loaded, the list is simply "None" and not useable.
 * When materials are loaded, the list lets the user pick None, any one for
 * whole actor colors, or 'Value Indexed' which says that each block and
 * cell gets to make its own choice via the indexed lookup table annotation
 * names.
 */

#ifndef vtkSMMaterialDomain_h
#define vtkSMMaterialDomain_h

#include "vtkRemotingServerManagerViewsModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkSMMaterialDomain : public vtkSMStringListDomain
{
public:
  static vtkSMMaterialDomain* New();
  vtkTypeMacro(vtkSMMaterialDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to get list of materials from materiallibrary singleton.
   */
  void Update(vtkSMProperty*) override;

protected:
  vtkSMMaterialDomain();
  ~vtkSMMaterialDomain() override;
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

private:
  vtkSMMaterialDomain(const vtkSMMaterialDomain&) = delete;
  void operator=(const vtkSMMaterialDomain&) = delete;

  void MaterialsUpdated(vtkObject* sender, unsigned long, void*);
};

#endif
