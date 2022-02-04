/*=========================================================================

  Program:   ParaView
  Module:    vtkClientSessionViews.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkClientSessionViews
 * @brief
 *
 */

#ifndef vtkClientSessionViews_h
#define vtkClientSessionViews_h

#include "vtkClientSession.h"
#include "vtkRemotingServerManagerViewsModule.h" // for exports

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkClientSessionViews : public vtkClientSession
{
public:
  static vtkClientSessionViews* New();
  vtkTypeMacro(vtkClientSessionViews, vtkClientSession);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkClientSessionViews();
  ~vtkClientSessionViews();

  void InitializeServiceEndpoint(vtkServiceEndpoint* endpoint) override;

private:
  vtkClientSessionViews(const vtkClientSessionViews&) = delete;
  void operator=(const vtkClientSessionViews&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
