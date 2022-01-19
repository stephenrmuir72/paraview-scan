/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOutputPort.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMOutputPort
 * @brief
 *
 */

#ifndef vtkSMOutputPort_h
#define vtkSMOutputPort_h

#include "vtkObject.h"
#include "vtkServerManagerCoreModule.h" // for exports

#include <memory> // for std::unique_ptr

class vtkSMProxy;
class vtkPVDataInformation;

class VTKSERVERMANAGERCORE_EXPORT vtkSMOutputPort : public vtkObject
{
public:
  static vtkSMOutputPort* New();
  vtkTypeMacro(vtkSMOutputPort, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Returns the proxy for the producer that this output port is for.
   */
  vtkSMProxy* GetProducer() const;
  ///@}

  ///@{
  /**
   * Returns the port index.
   */
  int GetIndex() const;
  ///@}

  /**
   * Returns data information for the data on this port.
   * This simply returns the currently available data information.
   */
  vtkPVDataInformation* GetDataInformation() const;

  ///@{
  /**
   * vtkSMProxy and vtkSMOutputPort form a reference loop and hence we
   * need garbage collection.
   */
  void Register(vtkObjectBase* o) override { this->RegisterInternal(o, true); }
  void UnRegister(vtkObjectBase* o) override { this->UnRegisterInternal(o, true); }
  ///@}

protected:
  vtkSMOutputPort();
  ~vtkSMOutputPort() override;

  /**
   * needed for garbage collection.
   */
  void ReportReferences(vtkGarbageCollector* collector) override;

  ///@{
  friend class vtkSMProxy;
  void SetProducer(vtkSMProxy* producer);
  void SetIndex(int index);
  void PipelineUpdated();
  ///@}

private:
  vtkSMOutputPort(const vtkSMOutputPort&) = delete;
  void operator=(const vtkSMOutputPort&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
