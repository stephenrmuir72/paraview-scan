/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonAPI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPVPythonAPI_h
#define vtkPVPythonAPI_h

#include "vtkObject.h"
#include "vtkPVPythonAPIModule.h" // For windows import/export of shared libraries

class VTKPVPYTHONAPI_EXPORT vtkPVPythonAPI : public vtkObject
{
public:
  vtkTypeMacro(vtkPVPythonAPI, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static void InitializeParaViewPython();

protected:
  vtkPVPythonAPI();
  ~vtkPVPythonAPI();

private:
  vtkPVPythonAPI(const vtkPVPythonAPI&) = delete;
  void operator=(const vtkPVPythonAPI&) = delete;
};

#endif
