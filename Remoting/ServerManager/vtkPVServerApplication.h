/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerApplication.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVServerApplication
 * @brief
 *
 */

#ifndef vtkPVServerApplication_h
#define vtkPVServerApplication_h

#include "vtkPVCoreApplication.h"

class vtkPVServerOptions;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVServerApplication : public vtkPVCoreApplication
{
public:
  static vtkPVServerApplication* New();
  vtkTypeMacro(vtkPVServerApplication, vtkPVCoreApplication);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the singleton once it has been created and initialized.
   */
  static vtkPVServerApplication* GetInstance();

  /**
   * Provides access to server options.
   */
  vtkPVServerOptions* GetOptions() const;

protected:
  vtkPVServerApplication();
  ~vtkPVServerApplication() override;

  /**
   * Creates a single vtkServerSession. A server application currently only
   * supports a unique server session.
   */
  bool InitializeInternal() override;

  void FinalizeInternal() override;

  /**
   * Returns an URL constructed using port / hostname specified in the
   * vtkPVServerOptions.
   */
  std::string GetEngineURL() const override;

  /**
   * Overridden to create vtkPVServerOptions.
   */
  vtkSmartPointer<vtkPVApplicationOptions> CreateOptions() const override;

private:
  vtkPVServerApplication(const vtkPVServerApplication&) = delete;
  void operator=(const vtkPVServerApplication&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
