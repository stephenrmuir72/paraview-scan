// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkChartRectangleRepresentation.h"

#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkRectangleItem.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartRectangleRepresentation);

//----------------------------------------------------------------------------
int vtkChartRectangleRepresentation::FillInputPortInformation(int port, vtkInformation* info)
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkChartRectangleRepresentation::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

//----------------------------------------------------------------------------
bool vtkChartRectangleRepresentation::AddToView(vtkView* view)
{
  if (vtkPVContextView* chartView = vtkPVContextView::SafeDownCast(view))
  {
    chartView->GetContextView()->GetScene()->AddItem(this->RectangleItem);
  }
}

//----------------------------------------------------------------------------
bool vtkChartRectangleRepresentation::RemoveFromView(vtkView* view)
{
  if (vtkPVContextView* chartView = vtkPVContextView::SafeDownCast(view))
  {
    chartView->GetContextView()->GetScene()->RemoveItem(this->RectangleItem);
  }
}

//----------------------------------------------------------------------------
void vtkChartRectangleRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
