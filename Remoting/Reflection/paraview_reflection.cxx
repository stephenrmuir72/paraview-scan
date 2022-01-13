#include "paraview_reflection.h"

#include "vtkObjectBase.h"
#include "vtkSphereSourceRXX.h"

void VTK_EXPORT paraview_reflection_initialize()
{
  vtkSphereSourceRXX();
}
