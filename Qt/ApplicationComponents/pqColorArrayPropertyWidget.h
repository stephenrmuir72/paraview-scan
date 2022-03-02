/*=========================================================================

   Program: ParaView
   Module:  pqColorArrayPropertyWidget.h

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
#ifndef pqColorArrayPropertyWidget_h
#define pqColorArrayPropertyWidget_h

#include "pqApplicationComponentsModule.h" // for exports
#include "pqPropertyWidget.h"
#include "pqSMProxy.h" // for pqSMProxy

#include <memory> // for std::unique_ptr

class PQAPPLICATIONCOMPONENTS_EXPORT pqColorArrayPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

  Q_PROPERTY(
    QPair<int, QString> colorArray READ colorArray WRITE setColorArray NOTIFY colorArrayChanged);
  Q_PROPERTY(pqSMProxy lookupTable READ lookupTable WRITE setLookupTable NOTIFY lookupTableChanged);
  Q_PROPERTY(int colorArrayComponent READ colorArrayComponent WRITE setColorArrayComponent NOTIFY
      colorArrayComponentChanged);
  Q_PROPERTY(int vectorMode READ vectorMode WRITE setVectorMode NOTIFY colorArrayComponentChanged);

public:
  explicit pqColorArrayPropertyWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqColorArrayPropertyWidget() override;

  ///@{
  /**
   * Get/Set the color array. This is 2 tuple where the first value is a string
   * that provides the array association while second value is the name of the
   * array.
   */
  QPair<int, QString> colorArray() const;
  void setColorArray(const QPair<int, QString>& value);
  ///@}

  ///@{
  /**
   * Get/set the lookup table being used.
   */
  pqSMProxy lookupTable() const;
  void setLookupTable(const pqSMProxy& proxy);
  ///@}

  ///@{
  /**
   * Get/Set the color array component being used.
   */
  int colorArrayComponent() const;
  void setColorArrayComponent(int component);
  ///@}

  ///@{
  /**
   * Get/Set vector mode. 0 means magnitude and 1 means use specified component number.
   */
  int vectorMode() const;
  void setVectorMode(int mode);
  ///@}

  /**
   * Overridden to reset transfer function ranges.
   */
  void apply() override;

Q_SIGNALS:
  void colorArrayChanged();
  void colorArrayComponentChanged();
  void lookupTableChanged();

private Q_SLOTS:
  void domainUpdated();

private:
  Q_DISABLE_COPY(pqColorArrayPropertyWidget);

  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
