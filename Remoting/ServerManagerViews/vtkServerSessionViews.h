/*=========================================================================

  Program:   ParaView
  Module:    vtkServerSessionViews.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkServerSessionViews
 * @brief
 *
 */

#ifndef vtkServerSessionViews_h
#define vtkServerSessionViews_h

#include "vtkRemotingServerManagerViewsModule.h" // for exports
#include "vtkServerSession.h"

class VTKREMOTINGSERVERMANAGERVIEWS_EXPORT vtkServerSessionViews : public vtkServerSession
{
public:
  static vtkServerSessionViews* New();
  vtkTypeMacro(vtkServerSessionViews, vtkServerSession);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkServerSessionViews();
  ~vtkServerSessionViews() override;

  void InitializeService(vtkService* service, vtkObjectStore* store) override;

private:
  vtkServerSessionViews(const vtkServerSessionViews&) = delete;
  void operator=(const vtkServerSessionViews&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
