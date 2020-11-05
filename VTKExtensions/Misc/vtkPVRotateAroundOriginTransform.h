/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRotateAroundOriginTransform

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVRotateAroundOriginTransform
 * @brief   baseclass for all ParaView vtkTransform class.
 *
 * vtkPVRotateAroundOriginTransform extend vtkTransform in the sens that it extend the vtkTransform
 * API with an origin of rotation and a rotation around it.
*/

#ifndef vtkPVTransform_h
#define vtkPVTransform_h

#include "vtkPVVTKExtensionsMiscModule.h" //needed for exports
#include "vtkTransform.h"
class vtkTransform;

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVRotateAroundOriginTransform : public vtkTransform
{
public:
  static vtkPVRotateAroundOriginTransform* New();
  vtkTypeMacro(vtkPVRotateAroundOriginTransform, vtkTransform);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set Origin of rotation.
   */
  void SetOriginOfRotation(double xyz[3]);
  void SetOriginOfRotation(double x, double y, double z);
  vtkGetVector3Macro(OriginOfRotation, double);
  //@}

  //@{
  /**
   * Get/Set Rotation for the transform.
   */
  void SetRotation(double xyz[3]);
  void SetRotation(double x, double y, double z);
  vtkGetVector3Macro(Rotation, double);
  //@}

protected:
  vtkPVRotateAroundOriginTransform();
  ~vtkPVRotateAroundOriginTransform() override;

  void UpdateMatrix();

  double Rotation[3];
  double OriginOfRotation[3];

  vtkTransform* AbsoluteTransform;

private:
  vtkPVRotateAroundOriginTransform(const vtkPVRotateAroundOriginTransform&) = delete;
  void operator=(const vtkPVRotateAroundOriginTransform&) = delete;
};

#endif
