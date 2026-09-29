// SPDX-FileCopyrightText: Copyright (c) Kitware.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVAnariRenderView.h"

#include "vtkAnariRenderWindow.h"

#include "vtkObjectFactory.h"

vtkObjectFactoryNewMacro(vtkPVAnariRenderView);

vtkPVAnariRenderView::vtkPVAnariRenderView()
  : vtkPVRenderView(false)
{
  vtkNew<vtkAnariRenderWindow> window;

  window->SetSize(this->GetSize());
  window->SetPosition(this->GetPosition());
  window->SetDPI(this->GetPPI());

  this->SetRenderWindow(window);
  this->InitializeRenderWindow();
}

void vtkPVAnariRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkPVAnariRenderView::SetEnableANARI(bool enable) {}

bool vtkPVAnariRenderView::GetEnableANARI()
{
  return true;
}
