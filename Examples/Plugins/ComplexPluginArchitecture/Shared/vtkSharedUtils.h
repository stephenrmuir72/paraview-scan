/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSharedUtils.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
/**
 * @class   vtkSharedUtils
 * @brief   provide shared utils
 */

#ifndef vtkSharedUtils_h
#define vtkSharedUtils_h

#include "SharedModule.h"

#include <vtkObject.h>

class SHARED_EXPORT vtkSharedUtils : public vtkObject
{
public:
  static vtkSharedUtils* New();
  vtkTypeMacro(vtkSharedUtils, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * A mathematical constant. This version is atan(1.0) * 4.0
   */
  static double Pi() { return 3.141592653589793; }

  //@{
  /**
   * Convert radians into degrees
   */
  static float DegreesFromRadians(float radians);
  static double DegreesFromRadians(double radians);
  //@}
};
#endif
