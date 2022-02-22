/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelayedSphereSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDelayedSphereSource
 * @brief   A sphere source with a user-defined delay. Usefull for simulating
 *          long-running filters.
 */

#ifndef vtkDelayedSphereSource_h
#define vtkDelayedSphereSource_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" // needed for exports

#include "vtkSetGet.h"
#include "vtkSphereSource.h"

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkDelayedSphereSource : public vtkSphereSource
{
public:
  ///@{
  /**
   * Standard methods for obtaining type information, and printing.
   */
  vtkTypeMacro(vtkDelayedSphereSource, vtkSphereSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Construct sphere with radius=0.5 and default resolution 8 in both Phi
   * and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
   */
  static vtkDelayedSphereSource* New();

  /** Time to spend (in s)  during the RequestData call*/
  vtkSetClampMacro(DataDelay, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(DataDelay, double);

  /** Time to spend (in s)  during the RequestData call*/
  vtkSetClampMacro(InformationDelay, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(InformationDelay, double);

protected:
  vtkDelayedSphereSource();
  ~vtkDelayedSphereSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double DataDelay;
  double InformationDelay;

private:
  vtkDelayedSphereSource(const vtkDelayedSphereSource&) = delete;
  void operator=(const vtkDelayedSphereSource&) = delete;
};

#endif
