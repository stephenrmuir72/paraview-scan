/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqVector3DWidget.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>

//-----------------------------------------------------------------------------
pqVector3DWidget::pqVector3DWidget(const QVector3D& value, QWidget* parent)
  : QWidget(parent)
{
  this->Vector = value;

  this->setAutoFillBackground(true);

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(1, 1, 1, 1);
  layout->setSpacing(1);
  this->setLayout(layout);

  QDoubleSpinBox* valEditX = new QDoubleSpinBox(this);
  valEditX->setValue(value.x());
  layout->addWidget(valEditX);

  QDoubleSpinBox* valEditY = new QDoubleSpinBox(this);
  valEditY->setValue(value.y());
  layout->addWidget(valEditY);

  QDoubleSpinBox* valEditZ = new QDoubleSpinBox(this);
  valEditZ->setValue(value.z());
  layout->addWidget(valEditZ);

  QObject::connect(valEditX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [this](double val) { this->setValue(0, val); });

  QObject::connect(valEditY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [this](double val) { this->setValue(1, val); });

  QObject::connect(valEditZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [this](double val) { this->setValue(2, val); });
}

//-----------------------------------------------------------------------------
const QVector3D& pqVector3DWidget::value() const
{
  return this->Vector;
}

//-----------------------------------------------------------------------------
void pqVector3DWidget::setValue(int index, float val)
{
  this->Vector[index] = val;
  emit this->valueChanged(this->Vector);
}
