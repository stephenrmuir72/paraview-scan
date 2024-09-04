// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkContext2DScalarBarActor2.h"

#include "vtkAxis.h"
#include "vtkBoundingRectContextDevice2D.h"
#include "vtkBrush.h"
#include "vtkColorTransferFunction.h"
#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextDevice2D.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkTextProperty.h"
#include "vtkTransform2D.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewport.h"

#include "vtkNetworkImageSource.h"
#include "vtkPVLogoSource.h"

#include <limits>
#include <map>

#if defined(_WIN32) && !defined(__CYGWIN__)
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkContext2DScalarBarActor2);

//----------------------------------------------------------------------------
vtkContext2DScalarBarActor2::vtkContext2DScalarBarActor2()
{
  cout << "NEW ACTOR USED !" << endl;
}

//----------------------------------------------------------------------------
bool vtkContext2DScalarBarActor2::Paint(vtkContext2D* painter)
{
  this->Texture->Print(std::cout);
  cout << "First range: " << this->FirstRange[0] << " " << this->FirstRange[1] << endl;
  cout << "Second range: " << this->SecondRange[0] << " " << this->SecondRange[1] << endl;
  cout << "Title: " << this->Title << endl;
  cout << "Title 2: " << this->Title2 << endl;
  
  return this->Superclass::Paint(painter);
}

//----------------------------------------------------------------------------
void vtkContext2DScalarBarActor2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}