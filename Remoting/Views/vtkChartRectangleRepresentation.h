// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkChartRectangleRepresentation_h
#define vtkChartRectangleRepresentation_h

#include "vtkNew.h" // for vtkNew
#include "vtkPVDataRepresentation.h"

class vtkRectangleItem;

class VTKREMOTINGVIEWS_EXPORT vtkChartRectangleRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkChartRectangleRepresentation* New();
  vtkTypeMacro(vtkChartRectangleRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkChartRectangleRepresentation() = default;
  ~vtkChartRectangleRepresentation() override = default;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Overridden to invoke vtkCommand::UpdateDataEvent.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  vtkNew<vtkRectangleItem> RectangleItem;

private:
  vtkChartRectangleRepresentation(const vtkChartRectangleRepresentation&) = delete;
  void operator=(const vtkChartRectangleRepresentation&) = delete;
};

#endif // vtkChartRectangleRepresentation_h
