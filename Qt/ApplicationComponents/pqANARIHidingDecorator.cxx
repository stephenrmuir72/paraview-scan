// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include <pqANARIHidingDecorator.h>

//-----------------------------------------------------------------------------
pqANARIHidingDecorator::pqANARIHidingDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  this->decoratorLogic->Initialize(config, parentObject->proxy());
}

//-----------------------------------------------------------------------------
bool pqANARIHidingDecorator::canShowWidget(bool showAdvanced) const
{
  return this->decoratorLogic->CanShow(showAdvanced);
}
