/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSteeringParametersProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMSteeringParametersProxy_h
#define vtkSMSteeringParametersProxy_h

#include "vtkSMSelfGeneratingSourceProxy.h"

#include <array>

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMSteeringParametersProxy
  : public vtkSMSelfGeneratingSourceProxy
{
public:
  static vtkSMSteeringParametersProxy* New();
  vtkTypeMacro(vtkSMSteeringParametersProxy, vtkSMSelfGeneratingSourceProxy);
  virtual ~vtkSMSteeringParametersProxy();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetParameter(const char* parameter, vtkIdType tupleId, size_t numComps, double* tuple);
  template <typename... Args>
  void SetParameter(const char* parameter, vtkIdType tupleId, Args... args)
  {
    std::array<double, sizeof...(Args)> tuple{ { args... } };
    this->SetParameter(parameter, tupleId, tuple.size(), tuple.data());
  }
  /*
  void SetParameter(const char* parameter, vtkIdType tupleId, double tuple1);
  void SetParameter(const char* parameter, vtkIdType tupleId, double tuple2a, double tuple2b);
  void SetParameter(const char* parameter, vtkIdType tupleId, double tuple3a, double tuple3b, double
  tuple3c);
  */
  // void UpdatePipeline() override;

  void CreateVTKObjects() override;

protected:
  vtkSMSteeringParametersProxy();

  class Internals;
  Internals* P;
};

#endif // vtkSMSteeringParametersProxy_h
