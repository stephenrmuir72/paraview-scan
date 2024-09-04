// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/** @class   vtkContext2DScalarBarActor2
 *  @brief   Custom scalar bar actor for ParaView that uses the Charts API.
 *
 * .SECTION Description
 *
 * vtkContext2DScalarBarActor2 is a custom scalar bar actor that uses the
 * Charts API for drawing calls. As the scalar bar actor is inherently 2D,
 * the Charts API offers a comparatively simpler way of implementing features
 * in the scalar bar actor than using vtkPolyData primitives.
 */

#ifndef vtkContext2DScalarBarActor2_h
#define vtkContext2DScalarBarActor2_h

#include "vtkContext2DScalarBarActor.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

#include "vtkCoordinate.h"   // for Position and Position 2
#include "vtkRect.h"         // for functions that return vtkRects
#include "vtkSmartPointer.h" // for smart pointers

#include "vtkTexture.h"

class vtkAxis;
class vtkColorLegend;
class vtkColorTransferFunctionItem;
class vtkContextActor;
class vtkContext2D;
class vtkContextScene;
class vtkDoubleArray;
class vtkImageData;
class vtkPoints2D;
class vtkScalarsToColors;
class vtkTextProperty;
class vtkViewport;

class VTKREMOTINGVIEWS_EXPORT vtkContext2DScalarBarActor2 : public vtkContext2DScalarBarActor
{
public:
  vtkTypeMacro(vtkContext2DScalarBarActor2, vtkContext2DScalarBarActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkContext2DScalarBarActor2* New();

protected:
  vtkContext2DScalarBarActor2();
  ~vtkContext2DScalarBarActor2() override = default;

  bool Paint(vtkContext2D* painter) override;

private:
  vtkContext2DScalarBarActor2(const vtkContext2DScalarBarActor2&) = delete;
  void operator=(const vtkContext2DScalarBarActor2&) = delete;
};

#endif // vtkContext2DScalarBarActor2_h
