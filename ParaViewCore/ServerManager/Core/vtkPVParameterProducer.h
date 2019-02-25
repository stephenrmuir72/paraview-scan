/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVParameterProducer.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVParameterProducer
 * @brief   specialized subclass of vtkPVTrivialProducer that
 * presents field data of a data object via a parameter API
 * that works with vtkSMSelfGeneratingSourceProxy.
 *
 * This allows a dataset to present field data as properties
 * in the property panel that can be edited by users.
 */

#ifndef vtkPVParameterProducer_h
#define vtkPVParameterProducer_h

#include "vtkPVTrivialProducer.h"

#include <array>

class vtkLiveInsituLink;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVParameterProducer : public vtkPVTrivialProducer
{
public:
  static vtkPVParameterProducer* New();
  vtkTypeMacro(vtkPVParameterProducer, vtkPVTrivialProducer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetObjectMacro(Link, vtkLiveInsituLink);
  virtual void SetLink(vtkLiveInsituLink* link);

  void SetParameter(const char* parameter, vtkIdType tupleId, size_t numComps, double* tuple);
  void SetParameter(const char* parameter, vtkIdType tupleId, const char* tupleValue);
  void SetParameter(const char* parameter, const char* tupleValue)
  {
    this->SetParameter(parameter, 0, tupleValue);
  }

  void SetParameter(const char* parameter, vtkIdType tupleId, double tuple1)
  {
    std::array<double, 1> tuple{ tuple1 };
    this->SetParameter(parameter, tupleId, 1, tuple.data());
  }
  void SetParameter(const char* parameter, vtkIdType tupleId, double tuple2a, double tuple2b)
  {
    std::array<double, 2> tuple{ tuple2a, tuple2b };
    this->SetParameter(parameter, tupleId, 2, tuple.data());
  }
  void SetParameter(
    const char* parameter, vtkIdType tupleId, double tuple3a, double tuple3b, double tuple3c)
  {
    std::array<double, 3> tuple{ tuple3a, tuple3b, tuple3c };
    this->SetParameter(parameter, tupleId, 3, tuple.data());
  }
  /* This is not getting CS-wrapped properly:
  template<typename... Args>
  void SetParameter(const char* parameter, vtkIdType tupleId, Args... args)
  {
    std::array<double, sizeof...(Args)> tuple{{args...}};
    this->SetParameter(parameter, tupleId, tuple.size(), tuple.data());
  }
  */

  vtkAbstractArray* GetParameter(const char* parameter);

protected:
  vtkPVParameterProducer();
  ~vtkPVParameterProducer() override;

  vtkLiveInsituLink* Link;

private:
  vtkPVParameterProducer(const vtkPVParameterProducer&) = delete;
  void operator=(const vtkPVParameterProducer&) = delete;
};

#endif
